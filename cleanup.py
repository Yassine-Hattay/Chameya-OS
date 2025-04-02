import os

# File extensions to delete
extensions = {".o"}

# Walk through the current directory and subdirectories
for root, _, files in os.walk("."):
    for filename in files:
        if os.path.splitext(filename)[1] in extensions:
            file_path = os.path.join(root, filename)
            os.remove(file_path)
            print(f"Deleted: {file_path}")

print("Cleanup complete.")
