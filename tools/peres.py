"""Bağımlılıksız küçük PE (Windows .exe/.dll/.msstyles) kaynak okuyucu.

    res = PEResources(open(path, "rb").read())
    res.items()            -> [(tür, ad, dil, bytes)]
    res.get("BITMAP", "NORMAL_BLUE_CAPTION")   -> bytes | None
    res.icon_groups()      -> {ad: ico dosyası bytes}
    res.cursor_groups()    -> {ad: cur dosyası bytes}
"""
import struct

RT = {1: "CURSOR", 2: "BITMAP", 3: "ICON", 4: "MENU", 5: "DIALOG", 6: "STRING",
      9: "ACCELERATOR", 10: "RCDATA", 12: "GROUP_CURSOR", 14: "GROUP_ICON",
      16: "VERSION", 23: "HTML", 24: "MANIFEST"}


class PEResources:
    def __init__(self, data: bytes):
        self.data = data
        e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
        if data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
            raise ValueError("PE değil")
        nsec = struct.unpack_from("<H", data, e_lfanew + 6)[0]
        opt_size = struct.unpack_from("<H", data, e_lfanew + 20)[0]
        opt = e_lfanew + 24
        magic = struct.unpack_from("<H", data, opt)[0]
        dd = opt + (96 if magic == 0x10B else 112)
        self.rsrc_rva = struct.unpack_from("<I", data, dd + 2 * 8)[0]
        self.sections = []
        sec = opt + opt_size
        for i in range(nsec):
            o = sec + i * 40
            vsize, va, rawsize, rawptr = struct.unpack_from("<IIII", data, o + 8)
            self.sections.append((va, max(vsize, rawsize), rawptr))
        self._items = []
        if self.rsrc_rva:
            self.base = self._rva2off(self.rsrc_rva)
            self._walk(0, [])

    def _rva2off(self, rva):
        for va, size, raw in self.sections:
            if va <= rva < va + size:
                return rva - va + raw
        raise ValueError(f"RVA {rva:#x} bölümde değil")

    def _name(self, val):
        if val & 0x80000000:
            o = self.base + (val & 0x7FFFFFFF)
            n = struct.unpack_from("<H", self.data, o)[0]
            return self.data[o + 2:o + 2 + n * 2].decode("utf-16-le")
        return val

    def _walk(self, off, path):
        o = self.base + off
        named, ids = struct.unpack_from("<HH", self.data, o + 12)
        for i in range(named + ids):
            name, ptr = struct.unpack_from("<II", self.data, o + 16 + i * 8)
            key = self._name(name)
            if ptr & 0x80000000:
                self._walk(ptr & 0x7FFFFFFF, path + [key])
            else:
                rva, size = struct.unpack_from("<II", self.data, self.base + ptr)
                start = self._rva2off(rva)
                t = path[0]
                t = RT.get(t, t) if isinstance(t, int) else t
                self._items.append((t, path[1], key, self.data[start:start + size]))

    def items(self):
        return list(self._items)

    def get(self, rtype, name, lang=None):
        name_u = name.upper() if isinstance(name, str) else name
        for t, n, l, d in self._items:
            nn = n.upper() if isinstance(n, str) else n
            if t == rtype and nn == name_u and (lang is None or l == lang):
                return d
        return None

    def names(self, rtype):
        return [n for t, n, l, d in self._items if t == rtype]

    # ------------------------------------------------------- simge/imleç
    def _group(self, gtype, etype, is_cursor):
        out = {}
        singles = {n: d for t, n, l, d in self._items if t == etype}
        for t, n, l, d in self._items:
            if t != gtype:
                continue
            _, rtype, count = struct.unpack_from("<HHH", d, 0)
            entries, blobs = [], []
            for i in range(count):
                e = d[6 + i * 14: 6 + (i + 1) * 14]
                if is_cursor:
                    w, h2, planes, bpp, size, rid = struct.unpack("<HHHHIH", e)
                    blob = singles.get(rid)
                    if blob is None:
                        continue
                    hx, hy = struct.unpack_from("<HH", blob, 0)
                    blob = blob[4:]
                    entries.append((w & 0xFF, (h2 // 2) & 0xFF, 0, 0, hx, hy, len(blob)))
                else:
                    w, h, cc, res, planes, bpp, size, rid = struct.unpack("<BBBBHHIH", e)
                    blob = singles.get(rid)
                    if blob is None:
                        continue
                    entries.append((w, h, cc, 0, planes, bpp, len(blob)))
                blobs.append(blob)
            hdr = struct.pack("<HHH", 0, 2 if is_cursor else 1, len(entries))
            off = 6 + 16 * len(entries)
            dirs = b""
            for e, b in zip(entries, blobs):
                dirs += struct.pack("<BBBBHHII", *e, off)
                off += len(b)
            out[n] = hdr + dirs + b"".join(blobs)
        return out

    def icon_groups(self):
        return self._group("GROUP_ICON", "ICON", False)

    def cursor_groups(self):
        return self._group("GROUP_CURSOR", "CURSOR", True)

    def strings(self, lang=None):
        """STRINGTABLE -> {id: metin}"""
        out = {}
        for t, n, l, d in self._items:
            if t != "STRING" or (lang is not None and l != lang):
                continue
            base = (n - 1) * 16
            o = 0
            for i in range(16):
                ln = struct.unpack_from("<H", d, o)[0]
                o += 2
                if ln:
                    out[base + i] = d[o:o + ln * 2].decode("utf-16-le")
                o += ln * 2
        return out
