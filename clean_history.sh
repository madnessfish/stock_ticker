#!/bin/bash

echo "This will remove all sensitive data from git history and force push."
echo "This is a destructive operation that will rewrite history!"
read -p "Are you sure? (y/n): " confirm

if [ "$confirm" != "y" ]; then
    echo "Cancelled"
    exit 1
fi

# Remove sensitive strings from all files in history
git filter-branch --force --tree-filter "
    find . -type f -name '*.ino' -exec sed -i '' \
        -e 's/TP-Link_FA0C/YOUR-WIFI-SSID/g' \
        -e 's/11460367/YOUR-WIFI-PASSWORD/g' \
        -e 's/VWVA5X980GXW0HEK/YOUR_API_KEY/g' {} +
" --tag-name-filter cat -- --all

# Clean up
rm -rf .git/refs/original/
git reflog expire --expire=now --all
git gc --prune=now --aggressive

echo "History cleaned! Now force push with:"
echo "git push origin --force --all"