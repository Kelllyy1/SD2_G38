# Iteration 1
# Partly works

# Sends the correct value to the Module model, but currently not the Cell module; working to fix that part
import json
import boto3
from math import ceil

client = boto3.client('iotsitewise')
MAX_BATCH_SIZE = 10

# These must match SiteWise property aliases exactly
MODULE_ALIAS_MAP = {
    "normal_cells": "Module001/normal_cells",
    "total_cells": "Module001/total_cells",
    "compromised_cells": "Module001/compromised_cells",
    "average_voltage": "Module001/average_voltage",
    "average_temperature": "Module001/average_temperature",
    "average_current": "Module001/average_current",
    "fault_count_normal": "Module001/fault_count_normal",
    "fault_over_current": "Module001/fault_over_current",
    "fault_over_discharge": "Module001/fault_over_discharge",
    "fault_overheating": "Module001/fault_overheating"
}

# Keys expected to be integers (for SiteWise property data types)
INTEGER_KEYS = {
    "normal_cells",
    "total_cells",
    "compromised_cells",
    "fault_count_normal",
    "fault_over_current",
    "fault_over_discharge",
    "fault_overheating"
}

def extract_cell_suffix(cell_id):
    parts = cell_id.split("-")
    return parts[-1] if len(parts) == 4 else None

def lambda_handler(event, context):
    try:
        payload = json.loads(event.get("body", json.dumps(event)))
        timestamp = int(payload.get("timeInSeconds", 0))
        quality = "GOOD"
        entries = []

        # ─── MODULE STATISTICS ─────────────────────────────────────
        stats = payload.get("statistics", {})
        fault_counts = stats.get("fault_counts", {})

        # Regular module stats
        for key in ["normal_cells", "total_cells", "compromised_cells", "average_voltage", "average_temperature", "average_current"]:
            value = stats.get(key)
            if value is not None and key in MODULE_ALIAS_MAP:
                value_type = "integerValue" if key in INTEGER_KEYS else "doubleValue"
                entries.append({
                    "entryId": f"module-{key}",
                    "propertyAlias": MODULE_ALIAS_MAP[key],
                    "propertyValues": [{
                        "value": { value_type: int(value) if value_type == "integerValue" else float(value) },
                        "timestamp": { "timeInSeconds": timestamp },
                        "quality": quality
                    }]
                })

        # Mapped fault count keys to SiteWise names
        fault_mapping = {
            "Normal": "fault_count_normal",
            "Over_current": "fault_over_current",
            "Over_discharge": "fault_over_discharge",
            "Overheating": "fault_overheating"
        }

        for fault_type, sitewise_key in fault_mapping.items():
            value = fault_counts.get(fault_type)
            if value is not None and sitewise_key in MODULE_ALIAS_MAP:
                entries.append({
                    "entryId": f"module-{sitewise_key}",
                    "propertyAlias": MODULE_ALIAS_MAP[sitewise_key],
                    "propertyValues": [{
                        "value": { "integerValue": int(value) },
                        "timestamp": { "timeInSeconds": timestamp },
                        "quality": quality
                    }]
                })

        # ─── COMPROMISED CELLS ─────────────────────────────────────
        for cell in payload.get("compromised_cells", []):
            cell_id = cell.get("id")
            suffix = extract_cell_suffix(cell_id)
            if not suffix:
                print(f"Skipping invalid cell ID: {cell_id}")
                continue

            asset_name = f"Cell-{suffix}"

            # Numeric measurements
            for field in ["voltage", "curr", "temperature"]:
                if field in cell:
                    entries.append({
                        "entryId": f"{asset_name}-{field}",
                        "propertyAlias": f"{asset_name}/{field}",
                        "propertyValues": [{
                            "value": { "doubleValue": float(cell[field]) },
                            "timestamp": { "timeInSeconds": timestamp },
                            "quality": quality
                        }]
                    })

            # Boolean for is_compromised
            is_compromised = str(cell.get("status", "")).strip().lower() == "compromised"
            entries.append({
                "entryId": f"{asset_name}-is_compromised",
                "propertyAlias": f"{asset_name}/is_compromised",
                "propertyValues": [{
                    "value": { "booleanValue": is_compromised },
                    "timestamp": { "timeInSeconds": timestamp },
                    "quality": quality
                }]
            })

            # String fields
            for field in ["status", "faults", "id"]:
                if field in cell:
                    entries.append({
                        "entryId": f"{asset_name}-{field}",
                        "propertyAlias": f"{asset_name}/{field}",
                        "propertyValues": [{
                            "value": { "stringValue": str(cell[field]) },
                            "timestamp": { "timeInSeconds": timestamp },
                            "quality": quality
                        }]
                    })

        # ─── SEND TO SITEWISE IN BATCHES ───────────────────────────
        if entries:
            total_batches = ceil(len(entries) / MAX_BATCH_SIZE)
            for i in range(total_batches):
                batch = entries[i * MAX_BATCH_SIZE:(i + 1) * MAX_BATCH_SIZE]
                response = client.batch_put_asset_property_value(entries=batch)
                print(f"Sent batch {i+1}/{total_batches}:", response)
        else:
            print("No valid entries to send to SiteWise.")

        return {
            "statusCode": 200,
            "body": json.dumps("Success")
        }

    except Exception as e:
        print("Error in Lambda:", str(e))
        return {
            "statusCode": 500,
            "body": json.dumps(f"Error: {str(e)}")
        }
