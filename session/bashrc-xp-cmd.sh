# >>> XPThemePlasma XP_CMD >>>
# Windows XP Komut İstemi görünümü (yalnızca Konsole "XP Komut İstemi" profilinde, XP_CMD=1)
if [[ -n "$XP_CMD" && $- == *i* ]]; then
  __xp_yol() {
    local p="$PWD"
    if [[ "$p" == "$HOME"* ]]; then p="C:\\Documents and Settings\\$USER${p#$HOME}"; else p="C:${p}"; fi
    p="${p//\//\\}"
    [[ "$p" == "C:" ]] && p="C:\\"
    printf '%s' "$p"
  }
  PS1='$(__xp_yol)>'
  if [[ -z "$XP_CMD_BANNER" ]]; then
    export XP_CMD_BANNER=1
    printf 'Microsoft Windows XP [Sürüm 5.1.2600]\n(C) Telif Hakkı 1985-2001 Microsoft Corp.\n\n'
  fi
  alias cls=clear dir='ls -la' copy=cp move=mv del=rm md=mkdir rd=rmdir type=cat ren=mv
  alias ver='printf "\nMicrosoft Windows XP [Sürüm 5.1.2600]\n"'
fi
# <<< XPThemePlasma XP_CMD <<<
