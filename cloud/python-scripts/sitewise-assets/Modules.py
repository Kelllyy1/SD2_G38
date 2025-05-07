# This script creates a new asset model in AWS IoT SiteWise for a "Module" that contains multiple "Cell" assets.
    # It also associates existing "Cell" assets with the "Module" asset model.
# Since the Module has a hierarchy of "has_cells", it can contain multiple "Cell" assets.
    # This script reads the cells ids from "cell_ids.txt" and associates them with the Module asset.

import boto3
import time

client = boto3.client('iotsitewise')

def get_module_model_info():
    models = client.list_asset_models()['assetModelSummaries']
    module_model_id = next((m['id'] for m in models if m['name'] == 'Module'), None)

    if not module_model_id:
        raise ValueError("Module model not found.")

    model_desc = client.describe_asset_model(assetModelId=module_model_id)
    hierarchy_id = next(h['id'] for h in model_desc['assetModelHierarchies'] if h['name'] == 'has_cells')

    return module_model_id, hierarchy_id

def get_cell_asset_ids():
    with open("cell_ids.txt") as f:
        return [line.strip() for line in f.readlines()]

def wait_for_asset_active(asset_id, timeout=60):
    print("Waiting for Module asset to become ACTIVE...")
    for _ in range(timeout):
        status = client.describe_asset(assetId=asset_id)['assetStatus']['state']
        if status == "ACTIVE":
            print("Module asset is ACTIVE.")
            return
        time.sleep(1)
    raise TimeoutError(f"Asset {asset_id} did not become ACTIVE in {timeout} seconds.")

def get_or_create_module_asset(module_model_id):
    existing_assets = client.list_assets(assetModelId=module_model_id)['assetSummaries']
    for asset in existing_assets:
        if asset['name'] == "BatteryModule001":
            print(f"Found existing Module: {asset['name']} | ID: {asset['id']}")
            return asset['id'], False

    response = client.create_asset(
        assetName="BatteryModule001",
        assetModelId=module_model_id
    )
    module_asset_id = response['assetId']
    print(f"Created Module asset: BatteryModule001 | ID: {module_asset_id}")
    wait_for_asset_active(module_asset_id)
    return module_asset_id, True

def associate_cells(module_asset_id, hierarchy_id, cell_asset_ids):
    for cell_id in cell_asset_ids:
        client.associate_assets(
            assetId=module_asset_id,
            hierarchyId=hierarchy_id,
            childAssetId=cell_id
        )
        print(f"Linked Cell ID {cell_id} to Module.")

if __name__ == "__main__":
    module_model_id, hierarchy_id = get_module_model_info()
    cell_asset_ids = get_cell_asset_ids()
    module_asset_id, is_new = get_or_create_module_asset(module_model_id)

    if not is_new:
        wait_for_asset_active(module_asset_id)

    associate_cells(module_asset_id, hierarchy_id, cell_asset_ids)
