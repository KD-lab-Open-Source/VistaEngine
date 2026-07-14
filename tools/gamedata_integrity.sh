#!/bin/bash
# GameData integrity check.
#
# The game can REWRITE its own data libraries (LibraryWrapperBase::saveLibrary,
# reached e.g. from a version bump in loadLibrary or a debug key), and off-Windows
# the text-archive writer has corrupted CP1251 Cyrillic to U+FFFD before. That
# silently breaks name lookups (a ParameterTypeReference falls back to key 0), so
# a corrupted file shows up as wrong gameplay numbers, not as an error.
#
# Usage:
#   tools/gamedata_integrity.sh baseline   # record hashes of the current GameData
#   tools/gamedata_integrity.sh check      # compare GameData against the baseline
#
# Files the game legitimately generates (logs, config, caches) are excluded.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA="$ROOT/GameData"
MANIFEST="$ROOT/tools/gamedata.sha256"

# Generated/rewritten at runtime by design; not evidence of corruption.
# Debug.dat is saved by DebugPrm on every ~Universe() (mission exit).
EXCLUDES='/(lst|_console\.log|iniFile\.cfg|\.DS_Store|Debug\.dat)$|/CacheData/'

manifest_now() {
	cd "$DATA" || exit 1
	# NUL-delimited throughout: plenty of asset paths contain spaces.
	find . -type f -print0 | grep -zEv "$EXCLUDES" | LC_ALL=C sort -z |
		xargs -0 shasum -a 256
}

case "${1:-check}" in
baseline)
	manifest_now > "$MANIFEST"
	echo "baseline written: $MANIFEST ($(wc -l < "$MANIFEST" | tr -d ' ') files)"
	;;
check)
	[ -f "$MANIFEST" ] || { echo "no baseline; run: $0 baseline"; exit 2; }
	diff <(manifest_now) "$MANIFEST" > /tmp/gamedata_integrity.diff 2>&1
	if [ $? -eq 0 ]; then
		echo "GameData OK ($(wc -l < "$MANIFEST" | tr -d ' ') files match baseline)"
	else
		echo "GameData CHANGED — the game rewrote shipped data:"
		# Show the changed paths (lines starting with '<' are the current state).
		grep '^[<>]' /tmp/gamedata_integrity.diff | awk '{print $NF}' | sort -u | sed 's/^/  /'
		echo
		echo "Restore with:  bsdtar -xf Perimeter2Steam.7z -C GameData <path>"
		exit 1
	fi
	;;
*)
	echo "usage: $0 {baseline|check}"; exit 2
	;;
esac
