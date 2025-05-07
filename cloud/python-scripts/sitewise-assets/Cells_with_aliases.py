
import boto3
import re
import sys
import time

client = boto3.client('iotsitewise')

def get_cell_model_id():
    models = client.list_asset_models()['assetModelSummaries']
    return next((m['id'] for m in models if m['name'] == 'Cell'), None)

def wait_for_asset_active(asset_id, timeout=60):
    for _ in range(timeout):
        status = client.describe_asset(assetId=asset_id)['assetStatus']['state']
        if status == "ACTIVE":
            return
        time.sleep(1)
    raise TimeoutError(f"Asset {asset_id} did not become ACTIVE in time.")

def get_module_asset_and_hierarchy(module_name):
    models = client.list_asset_models()['assetModelSummaries']
    module_model_id = next((m['id'] for m in models if m['name'] == 'Module'), None)
    if not module_model_id:
        raise ValueError("Module model not found.")

    paginator = client.get_paginator('list_assets')
    assets = []
    for page in paginator.paginate(assetModelId=module_model_id):
        assets.extend(page['assetSummaries'])

    module_asset = next((a for a in assets if a['name'] == module_name), None)
    if not module_asset:
        raise ValueError(f"Module asset named '{module_name}' not found.")

    model_desc = client.describe_asset_model(assetModelId=module_model_id)
    hierarchy_id = next(h['id'] for h in model_desc['assetModelHierarchies'] if h['name'] == 'has_cells')

    return module_asset['id'], hierarchy_id

def get_last_cell_index_for_module(module_asset_id, hierarchy_id):
    associated = []
    paginator = client.get_paginator('list_associated_assets')
    for page in paginator.paginate(assetId=module_asset_id, hierarchyId=hierarchy_id):
        associated.extend(page['assetSummaries'])

    numbers = []
    for asset in associated:
        match = re.search(r"Cell-C(\d+)", asset['name'])
        if match:
            numbers.append(int(match.group(1)))
    return max(numbers) if numbers else 0

def set_property_aliases(asset_id, asset_name):
    asset = client.describe_asset(assetId=asset_id)
    for prop in asset['assetProperties']:
        prop_name = prop['name']
        prop_id = prop['id']
        prop_type = list(prop['type'].keys())[0] if 'type' in prop and prop['type'] else None

        if prop_type != 'measurement':
            print(f"⚠️ Skipped '{prop_name}' — not a measurement.")
            continue

        alias = f"{asset_name}/{prop_name}"
        client.update_asset_property(
            assetId=asset_id,
            propertyId=prop_id,
            propertyAlias=alias
        )
        print(f"🔗 Set alias: {alias}")

def create_and_link_cells(count, module_name):
    cell_model_id = get_cell_model_id()
    module_id, hierarchy_id = get_module_asset_and_hierarchy(module_name)
    last_index = get_last_cell_index_for_module(module_id, hierarchy_id)

    cell_ids = []
    cell_names = []

    for i in range(count):
        num = last_index + i + 1
        name = f"Cell-C{num:03d}"
        response = client.create_asset(
            assetName=name,
            assetModelId=cell_model_id
        )
        aid = response['assetId']
        print(f"🛠 Created Cell asset: {name} | ID: {aid}")
        cell_ids.append(aid)
        cell_names.append(name)

    print("⏳ Waiting for all cells to become ACTIVE...")
    for aid, name in zip(cell_ids, cell_names):
        wait_for_asset_active(aid)
        print(f"🟢 {name} is ACTIVE")

    for aid, name in zip(cell_ids, cell_names):
        set_property_aliases(aid, name)
        client.associate_assets(assetId=module_id, hierarchyId=hierarchy_id, childAssetId=aid)
        print(f"✅ Linked {name} to Module {module_name}")

    with open("cell_ids.txt", "w") as f:
        for aid in cell_ids:
            f.write(aid + "\n")

if __name__ == "__main__":
    if len(sys.argv) != 3 or not sys.argv[1].isdigit():
        print("Usage: python create_cells.py <number_of_cells> <ModuleName>")
        sys.exit(1)

    num_cells = int(sys.argv[1])
    module_name = sys.argv[2]
    create_and_link_cells(num_cells, module_name)
