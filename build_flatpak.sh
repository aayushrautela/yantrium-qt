#!/bin/bash

# Build script for Yantrium Flatpak
# This script builds a Flatpak package locally

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Yantrium Flatpak Build Script ===${NC}"

# Check if flatpak-builder is installed
if ! command -v flatpak-builder &> /dev/null; then
    echo -e "${RED}Error: flatpak-builder is not installed!${NC}"
    echo "Install it with: sudo dnf install flatpak flatpak-builder"
    exit 1
fi

# Load API keys from build.sh if it exists
if [ -f build.sh ]; then
    echo -e "${GREEN}Loading API keys from build.sh...${NC}"
    source <(grep -E '^TMDB_API_KEY=|^OMDB_API_KEY=|^TRAKT_CLIENT_ID=|^TRAKT_CLIENT_SECRET=' build.sh | sed 's/^/export /')
else
    echo -e "${YELLOW}Warning: build.sh not found. API keys must be set as environment variables.${NC}"
fi

# Check if API keys are set
if [ -z "$TMDB_API_KEY" ]; then
    echo -e "${YELLOW}Warning: TMDB_API_KEY is not set!${NC}"
    echo "Set it via: export TMDB_API_KEY='your_key'"
fi

if [ -z "$TRAKT_CLIENT_ID" ]; then
    echo -e "${YELLOW}Warning: TRAKT_CLIENT_ID is not set!${NC}"
    echo "Set it via: export TRAKT_CLIENT_ID='your_id'"
fi

if [ -z "$TRAKT_CLIENT_SECRET" ]; then
    echo -e "${YELLOW}Warning: TRAKT_CLIENT_SECRET is not set!${NC}"
    echo "Set it via: export TRAKT_CLIENT_SECRET='your_secret'"
fi

# Check if KDE runtime is installed
if ! flatpak list | grep -q "org.kde.Platform"; then
    echo -e "${GREEN}Installing KDE Platform runtime...${NC}"
    flatpak install -y flathub org.kde.Platform//6.6 org.kde.Sdk//6.6
fi

# Build the Flatpak
echo -e "${GREEN}Building Flatpak...${NC}"
flatpak-builder \
    --force-clean \
    --install \
    --user \
    build-dir \
    org.yantrium.Yantrium.yml

echo -e "${GREEN}=== Build Complete! ===${NC}"
echo ""
echo "To run the application:"
echo "  flatpak run org.yantrium.Yantrium"
echo ""
echo "To create a bundle for distribution:"
echo "  flatpak-builder --repo=repo --force-clean build-dir org.yantrium.Yantrium.yml"
echo "  flatpak build-bundle repo yantrium.flatpak org.yantrium.Yantrium"

