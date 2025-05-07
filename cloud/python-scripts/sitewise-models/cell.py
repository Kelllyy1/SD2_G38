# This script creates an Asset Model for a Cell in AWS IoT SiteWise.
# It defines the properties of the Cell, such as voltage, current, temperature, status, faults, and whether it is compromised.

import boto3

# Initialize SiteWise client
client = boto3.client('iotsitewise')

# Function to create the "Cell" Asset Model
def createCellModel():
    cell_model = client.create_asset_model(
        assetModelName="Cell",
        assetModelProperties=[
            {
                'name': 'id',
                'dataType': 'STRING',
                'type': {'attribute': {}}
            },
            {
                'name': 'voltage',
                'dataType': 'DOUBLE',
                'unit': 'Volts',
                'type': {'measurement': {}}
            },
            {
                'name': 'curr',
                'dataType': 'DOUBLE',
                'unit': 'Amps',
                'type': {'measurement': {}}
            },
            {
                'name': 'temperature',
                'dataType': 'DOUBLE',
                'unit': 'Celsius',
                'type': {'measurement': {}}
            },
            {
                'name': 'status',
                'dataType': 'STRING',
                'type': {'attribute': {}}
            },
            {
                'name': 'faults',
                'dataType': 'STRING',
                'type': {'attribute': {}}
            },
            {
                'name': 'is_compromised',
                'dataType': 'BOOLEAN',
                'type': {'attribute': {}}
            }
        ]
    )

    cell_model_id = cell_model['assetModelId']
    return cell_model_id

# Call the function
cell_model_id = createCellModel()
print(f"Created 'Cell' model with ID: {cell_model_id}")