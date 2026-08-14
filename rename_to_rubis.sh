#!/bin/bash
# ============================================================
#  Script de renommage : Prism Launcher → Rubis Launcher
#  Lance depuis la RACINE de ton fork Prism
#  Usage : bash rename_to_rubis.sh
# ============================================================

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log()  { echo -e "${GREEN}[OK]${NC} $1"; }
warn() { echo -e "${YELLOW}[??]${NC} $1"; }
fail() { echo -e "${RED}[ERREUR]${NC} $1"; }

echo ""
echo "  ======================================"
echo "   Rubis Launcher — script de renommage"
echo "  ======================================"
echo ""

# Vérifie qu'on est bien dans un repo Prism
if [ ! -f "CMakeLists.txt" ]; then
  fail "CMakeLists.txt introuvable. Lance ce script depuis la racine du repo."
  exit 1
fi

if ! grep -q "PrismLauncher\|Prism Launcher" CMakeLists.txt 2>/dev/null; then
  warn "Ce repo ne semble pas être un fork Prism standard — vérifie CMakeLists.txt"
fi

echo "Début des remplacements..."
echo ""

# ------------------------------------------------------------
# Fonction utilitaire : remplace dans un fichier si il existe
# ------------------------------------------------------------
replace_in_file() {
  local file="$1"
  local from="$2"
  local to="$3"
  if [ -f "$file" ]; then
    sed -i "s|$from|$to|g" "$file"
    log "$file"
  else
    warn "$file introuvable (ignoré)"
  fi
}

# ------------------------------------------------------------
# 1. CMakeLists.txt
# ------------------------------------------------------------
echo "--- CMakeLists.txt ---"
replace_in_file "CMakeLists.txt" "Prism Launcher" "Rubis Launcher"
replace_in_file "CMakeLists.txt" "PrismLauncher" "RubisLauncher"
replace_in_file "CMakeLists.txt" "prismlauncher" "rubislauncher"
replace_in_file "CMakeLists.txt" "prismlauncher\.org" "rubislauncher.net"

# ------------------------------------------------------------
# 2. buildconfig/BuildConfig.cpp
# ------------------------------------------------------------
echo ""
echo "--- buildconfig/ ---"
replace_in_file "buildconfig/BuildConfig.cpp" "Prism Launcher" "Rubis Launcher"
replace_in_file "buildconfig/BuildConfig.cpp" "PrismLauncher" "RubisLauncher"
replace_in_file "buildconfig/BuildConfig.cpp" "prismlauncher\.org" "rubislauncher.net"
replace_in_file "buildconfig/BuildConfig.cpp" "github\.com/PrismLauncher/PrismLauncher" "github.com/TON_PSEUDO/RubisLauncher"
replace_in_file "buildconfig/BuildConfig.h"   "Prism Launcher" "Rubis Launcher"
replace_in_file "buildconfig/BuildConfig.h"   "PrismLauncher" "RubisLauncher"

# ------------------------------------------------------------
# 3. UI — MainWindow & AboutDialog
# ------------------------------------------------------------
echo ""
echo "--- launcher/ui/ ---"
replace_in_file "launcher/ui/MainWindow.cpp"              "Prism Launcher" "Rubis Launcher"
replace_in_file "launcher/ui/MainWindow.cpp"              "PrismLauncher"  "RubisLauncher"
replace_in_file "launcher/ui/dialogs/AboutDialog.cpp"     "Prism Launcher" "Rubis Launcher"
replace_in_file "launcher/ui/dialogs/AboutDialog.cpp"     "PrismLauncher"  "RubisLauncher"
replace_in_file "launcher/ui/dialogs/AboutDialog.cpp"     "prismlauncher\.org" "rubislauncher.net"

# ------------------------------------------------------------
# 4. program_info — manifests & packaging
# ------------------------------------------------------------
echo ""
echo "--- program_info/ ---"
replace_in_file "program_info/win.manifest"               "Prism Launcher" "Rubis Launcher"
replace_in_file "program_info/win.manifest"               "PrismLauncher"  "RubisLauncher"

# NSIS (installeur Windows)
for f in program_info/nsis/*.nsi program_info/nsis/*.nsi.in program_info/nsis/*.nsh; do
  [ -f "$f" ] && {
    sed -i "s|Prism Launcher|Rubis Launcher|g" "$f"
    sed -i "s|PrismLauncher|RubisLauncher|g" "$f"
    sed -i "s|prismlauncher|rubislauncher|g" "$f"
    log "$f"
  }
done

# .desktop Linux
DESKTOP_OLD="program_info/org.prismlauncher.PrismLauncher.desktop"
DESKTOP_NEW="program_info/org.rubislauncher.RubisLauncher.desktop"
if [ -f "$DESKTOP_OLD" ]; then
  cp "$DESKTOP_OLD" "$DESKTOP_NEW"
  sed -i "s|Prism Launcher|Rubis Launcher|g" "$DESKTOP_NEW"
  sed -i "s|PrismLauncher|RubisLauncher|g" "$DESKTOP_NEW"
  sed -i "s|prismlauncher|rubislauncher|g" "$DESKTOP_NEW"
  rm "$DESKTOP_OLD"
  log "$DESKTOP_NEW (ancien fichier supprimé)"
else
  warn "$DESKTOP_OLD introuvable (ignoré)"
fi

# AppStream / metainfo
for f in program_info/*.xml program_info/*.metainfo.xml program_info/*.appdata.xml; do
  [ -f "$f" ] && {
    sed -i "s|Prism Launcher|Rubis Launcher|g" "$f"
    sed -i "s|PrismLauncher|RubisLauncher|g" "$f"
    sed -i "s|prismlauncher\.org|rubislauncher.net|g" "$f"
    log "$f"
  }
done

# ------------------------------------------------------------
# 5. Scan global — cherche les "Prism" restants dans les sources
# ------------------------------------------------------------
echo ""
echo "--- Scan des occurrences restantes ---"
REMAINING=$(grep -r "Prism\|prismlauncher" \
  --include="*.cpp" --include="*.h" --include="*.cmake" \
  --include="*.txt" --include="*.nsi" --include="*.xml" \
  --include="*.desktop" --include="*.json" \
  -l 2>/dev/null | grep -v ".git/" | grep -v "rename_to_rubis.sh" || true)

if [ -z "$REMAINING" ]; then
  log "Aucune occurrence restante trouvée."
else
  warn "Occurrences 'Prism' encore présentes dans ces fichiers :"
  echo "$REMAINING" | sed 's/^/    /'
  echo ""
  warn "Vérifie-les manuellement (certains peuvent être des commentaires légitimes)."
fi

# ------------------------------------------------------------
# Résumé final
# ------------------------------------------------------------
echo ""
echo "  ======================================"
echo -e "  ${GREEN}Renommage terminé !${NC}"
echo "  ======================================"
echo ""
echo "  Prochaines étapes :"
echo "  1. Remplace github.com/TON_PSEUDO dans BuildConfig.cpp"
echo "  2. Copie ton logo dans program_info/ et launcher/resources/"
echo "  3. Lance : cmake -S . -B build && cmake --build build"
echo ""
