#!/bin/bash

# Build script for Yantrium Flatpak
# AUTOMATICALLY INJECTS API KEYS INTO MANIFEST

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Yantrium Flatpak Build Script ===${NC}"

# Check for required tools
for tool in flatpak-builder envsubst; do
    if ! command -v $tool &> /dev/null; then
        echo -e "${RED}Error: $tool is not installed!${NC}"
        echo "Install it via: sudo dnf install flatpak-builder gettext"
        exit 1
    fi
done

# Load API keys
if [ -f build.sh ]; then
    echo -e "${GREEN}Loading API keys from build.sh...${NC}"
    source <(grep -E '^TMDB_API_KEY=|^OMDB_API_KEY=|^TRAKT_CLIENT_ID=|^TRAKT_CLIENT_SECRET=' build.sh | sed 's/^/export /')
else
    echo -e "${YELLOW}Warning: build.sh not found. Using current environment variables.${NC}"
fi

# Verify keys exist
if [ -z "$TMDB_API_KEY" ]; then
    echo -e "${RED}Error: TMDB_API_KEY is missing! Build will fail.${NC}"
    exit 1
fi

# Export variables so envsubst can see them
export TMDB_API_KEY
export TRAKT_CLIENT_ID
export TRAKT_CLIENT_SECRET
export OMDB_API_KEY

# Check runtime
if ! flatpak list | grep -q "org.kde.Platform.*6.10"; then
    echo -e "${GREEN}Installing KDE Platform runtime 6.10...${NC}"
    flatpak install -y flathub org.kde.Platform//6.10 org.kde.Sdk//6.10
fi

# --- STEP 1: INJECT KEYS INTO MANIFEST ---
echo -e "${GREEN}Injecting API keys into temporary manifest...${NC}"
# We assume your source manifest is named org.yantrium.Yantrium.yml
# We generate a temporary file named org.yantrium.Yantrium.filled.yml
envsubst < org.yantrium.Yantrium.yml > org.yantrium.Yantrium.filled.yml

# --- STEP 2: BUILD ---
echo -e "${GREEN}Building Flatpak...${NC}"
flatpak-builder \
    --force-clean \
    --repo=repo \
    --install \
    --user \
    build-dir \
    org.yantrium.Yantrium.filled.yml  # <--- Note: We build the FILLED file

# --- STEP 3: BUNDLE ---
echo -e "${GREEN}Creating bundle...${NC}"
flatpak build-bundle repo yantrium.flatpak org.yantrium.Yantrium

# --- CLEANUP ---
rm org.yantrium.Yantrium.filled.yml

echo -e "${GREEN}=== Build Complete! ===${NC}"
echo "Run with: flatpak run org.yantrium.Yantrium"