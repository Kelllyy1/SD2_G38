
import boto3
import re
import sys
import time

client = boto3.client('iotsitewise')

def get_cell_model_id():
    models = client.list_asset_models()['assetModelSummaries']
    return next((m['id'] for m in models if m['name'] == 'Cell'), None)

def get_existing_cell_assets():
    model_id = get_cell_model_id()
    assets = client.list_assets(assetModelId=model_id)['assetSummaries']
    return [a for a in assets if a['name'].startswith("Cell-C")]

def get_last_cell_number(cell_assets):
    numbers = []
    for asset in cell_assets:
        match = re.search(r"Cell-C(\d+)", asset['name'])
        if match:
            numbers.append(int(match.group(1)))
    return max(numbers) if numbers else 0

def wait_for_asset_active(asset_id, timeout=60):
    for _ in range(timeout):
        state = client.describe_asset(assetId=asset_id)['assetStatus']['state']
        if state == "ACTIVE":
            return
        time.sleep(1)
    raise TimeoutError(f"Asset {asset_id} did not become ACTIVE in time.")

def get_module_asset_and_hierarchy(module_name):
    module_asset = next((a for a in client.list_assets()['assetSummaries'] if a['name'] == module_name), None)
    if not module_asset:
        raise ValueError(f"Module asset named '{module_name}' not found.")
    model_id = client.describe_asset(assetId=module_asset['id'])['assetModelId']
    model = client.describe_asset_model(assetModelId=model_id)
    hierarchy_id = next(h['id'] for h in model['assetModelHierarchies'] if h['name'] == 'has_cells')
    return module_asset['id'], hierarchy_id

def create_and_link_cells(count, module_name):
    cell_model_id = get_cell_model_id()
    existing_assets = get_existing_cell_assets()
    last_index = get_last_cell_number(existing_assets)

    module_id, hierarchy_id = get_module_asset_and_hierarchy(module_name)
    cell_ids = []

    for i in range(count):
        num = last_index + i + 1
        name = f"Cell-C{num:03d}"
        response = client.create_asset(
            assetName=name,
            assetModelId=cell_model_id
        )
        aid = response['assetId']
        wait_for_asset_active(aid)
        client.associate_assets(assetId=module_id, hierarchyId=hierarchy_id, childAssetId=aid)
        print(f"✅ Created & linked: {name} | ID: {aid}")
        cell_ids.append(aid)

    with open("cell_ids.txt", "w") as f:
        for aid in cell_ids:
            f.write(aid + "\n")

if __name__ == "__main__":
    if len(sys.argv) != 3 or not sys.argv[1].isdigit():
        print("Usage: python Cells.py <number_of_cells> <ModuleName>")
        sys.exit(1)

    count = int(sys.argv[1])
    module_name = sys.argv[2]
    create_and_link_cells(count, module_name)



# Iteration 1
# # This script creates an Asset Model for a Cell in AWS IoT SiteWise.
# # It defines the properties of the Cell, such as voltage, current, temperature, status, faults, and whether it is compromised.

# import boto3

# # Initialize SiteWise client
# client = boto3.client('iotsitewise')

# # Function to create the "Cell" Asset Model
# def createCellModel():
#     cell_model = client.create_asset_model(
#         assetModelName="Cell",
#         assetModelProperties=[
#             {
#                 'name': 'id',
#                 'dataType': 'STRING',
#                 'type': {'attribute': {}}
#             },
#             {
#                 'name': 'voltage',
#                 'dataType': 'DOUBLE',
#                 'unit': 'Volts',
#                 'type': {'measurement': {}}
#             },
#             {
#                 'name': 'curr',
#                 'dataType': 'DOUBLE',
#                 'unit': 'Amps',
#                 'type': {'measurement': {}}
#             },
#             {
#                 'name': 'temperature',
#                 'dataType': 'DOUBLE',
#                 'unit': 'Celsius',
#                 'type': {'measurement': {}}
#             },
#             {
#                 'name': 'status',
#                 'dataType': 'STRING',
#                 'type': {'attribute': {}}
#             },
#             {
#                 'name': 'faults',
#                 'dataType': 'STRING',
#                 'type': {'attribute': {}}
#             },
#             {
#                 'name': 'is_compromised',
#                 'dataType': 'BOOLEAN',
#                 'type': {'attribute': {}}
#             }
#         ]
#     )

#     cell_model_id = cell_model['assetModelId']
#     return cell_model_id

# # Call the function
# cell_model_id = createCellModel()
# print(f"Created 'Cell' model with ID: {cell_model_id}")