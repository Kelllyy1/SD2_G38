# This script creates a new asset model in AWS IoT SiteWise for a "Module" that contains multiple "Cell" assets [so that the process doesn't have to be done manually in sitewise - it's great for scalability].

import boto3

# Initialize SiteWise client
client = boto3.client('iotsitewise')

# Function to find the "Cell" Asset Model ID
def findCellId():
    models = client.list_asset_models()['assetModelSummaries']
    cell_model_id = next((m['id'] for m in models if m['name'] == 'Cell'), None)

    if not cell_model_id:
        raise ValueError("Cell model not found!")

    print(f"Using Cell Model ID: {cell_model_id}")
    return cell_model_id

# Function to create the "Module" Asset Model with hierarchy referencing "Cell"
def createCellmodel(cell_model_id):
    response = client.create_asset_model(
        assetModelName="Module",
        assetModelProperties=[
            {
                'name': 'average_current',
                'dataType': 'DOUBLE',
                'unit': 'Amps',
                'type': {'measurement': {}}
            },
            {
                'name': 'average_voltage',
                'dataType': 'DOUBLE',
                'unit': 'Volts',
                'type': {'measurement': {}}
            },
            {
                'name': 'average_temperature',
                'dataType': 'DOUBLE',
                'unit': 'Celsius',
                'type': {'measurement': {}}
            },
            {
                'name': 'total_cells',
                'dataType': 'INTEGER',
                'type': {'measurement': {}}
            },
            {
                'name': 'normal_cells',
                'dataType': 'INTEGER',
                'type': {'measurement': {}}
            },
            {
                'name': 'compromised_cells',
                'dataType': 'INTEGER',
                'type': {'measurement': {}}
            },
            {
                'name': 'fault_count_normal',
                'dataType': 'INTEGER',
                'type': {'measurement': {}}
            },
            {
                'name': 'fault_over_current',
                'dataType': 'INTEGER',
                'type': {'measurement': {}}
            },
            {
                'name': 'fault_overheating',
                'dataType': 'INTEGER',
                'type': {'measurement': {}}
            },
            {
                'name': 'fault_over_discharge',
                'dataType': 'INTEGER',
                'type': {'measurement': {}}
            }
        ],
        assetModelHierarchies=[
            {
                'name': 'has_cells',
                'childAssetModelId': cell_model_id

            }
        ]
    )
    module_model = response['assetModelId']
    return module_model

# Call the functions
cell_model_id = findCellId()
module_model = createCellmodel(cell_model_id)
print(f"Created 'Module' model with ID: {module_model}")
