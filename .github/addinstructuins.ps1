# Define your source and destination paths
$sourceDir = "F:\Projects\Unreal\5.7\ArchVis\.github\skills"
$destDir = "F:\Projects\Unreal\5.7\ArchVis\.github\instructions"

# Create the destination folder if it doesn't exist
if (!(Test-Path -Path $destDir)) {
    New-Item -ItemType Directory -Path $destDir
}

# Get all skill.md files in subdirectories
Get-ChildItem -Path $sourceDir -Filter "skill.md" -Recurse | ForEach-Object {
    
    # Get the name of the folder containing the file
    $parentName = $_.Directory.Name
    
    # Define the new filename
    $newName = "$parentName.instructions.md"
    
    # Define the full destination path
    $targetPath = Join-Path -ChildPath $newName -Path $destDir
    
    # Copy the file
    Copy-Item -Path $_.FullName -Destination $targetPath
    
    Write-Host "Copied: $($_.FullName) -> $targetPath"
}