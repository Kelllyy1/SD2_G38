# This script creates multiple "Cell" assets in AWS IoT SiteWise. It checks for existing assets, determines the last used number, and creates new assets with incremented numbers.
# The created asset IDs are saved to a file called "cell_ids.txt".

import boto3
import re
import sys

client = boto3.client('iotsitewise')

def get_cell_model_id():
    models = client.list_asset_models()['assetModelSummaries']
    return next((m['id'] for m in models if m['name'] == 'Cell'), None)

def get_existing_cell_assets():
    assets = client.list_assets(assetModelId=get_cell_model_id())['assetSummaries']
    cell_assets = [a for a in assets if a['name'].startswith("Cell-C")]
    return cell_assets

def get_last_cell_number(cell_assets):
    numbers = []
    for asset in cell_assets:
        match = re.search(r"Cell-C(\d+)", asset['name'])
        if match:
            numbers.append(int(match.group(1)))
    return max(numbers) if numbers else 0

def create_cell_assets(cell_model_id, count, start_index):
    asset_ids = []

    for i in range(count):
        num = start_index + i + 1
        asset_name = f"Cell-C{num:03d}"
        response = client.create_asset(
            assetName=asset_name,
            assetModelId=cell_model_id
        )
        asset_id = response['assetId']
        print(f"Created Cell asset: {asset_name} | ID: {asset_id}")
        asset_ids.append(asset_id)

    with open("cell_ids.txt", "w") as f:
        for aid in asset_ids:
            f.write(aid + "\n")

if __name__ == "__main__":
    if len(sys.argv) != 2 or not sys.argv[1].isdigit():
        print("Usage: python create_cells.py <number_of_cells>")
        sys.exit(1)

    num_cells = int(sys.argv[1])
    cell_model_id = get_cell_model_id()

    if not cell_model_id:
        print("Cell model not found.")
        sys.exit(1)

    existing_assets = get_existing_cell_assets()
    last_index = get_last_cell_number(existing_assets)

    create_cell_assets(cell_model_id, num_cells, last_index)
