import json
import boto3
from math import ceil

client = boto3.client('iotsitewise')
MAX_BATCH_SIZE = 10

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

def lambda_handler(event, context):
    try:
        payload = json.loads(event.get("body", json.dumps(event)))
        timestamp = int(payload.get("timeInSeconds", 0))
        quality = "GOOD"
        entries = []

        # ─── EXTRACT MODULE ID DYNAMICALLY ─────────────────────────
        compromised_cells = payload.get("compromised_cells", [])
        default_module = "Module001"
        module_id = default_module

        if compromised_cells:
            raw_id = compromised_cells[0].get("id", "")
            parts = raw_id.split("-")
            if len(parts) == 4:
                module_id = f"Module{parts[2]}"  # e.g. M001 → ModuleM001

        # ─── BUILD MODULE ALIAS MAP BASED ON DYNAMIC MODULE ID ─────
        MODULE_ALIAS_MAP = {
            "normal_cells": f"{module_id}/normal_cells",
            "total_cells": f"{module_id}/total_cells",
            "compromised_cells": f"{module_id}/compromised_cells",
            "average_voltage": f"{module_id}/average_voltage",
            "average_temperature": f"{module_id}/average_temperature",
            "average_current": f"{module_id}/average_current",
            "fault_count_normal": f"{module_id}/fault_count_normal",
            "fault_over_current": f"{module_id}/fault_over_current",
            "fault_over_discharge": f"{module_id}/fault_over_discharge",
            "fault_overheating": f"{module_id}/fault_overheating"
        }

        # ─── MODULE STATISTICS ─────────────────────────────────────
        stats = payload.get("statistics", {})
        fault_counts = stats.get("fault_counts", {})

        for key in ["normal_cells", "total_cells", "compromised_cells", "average_voltage", "average_temperature", "average_current"]:
            value = stats.get(key)
            if value is not None and key in MODULE_ALIAS_MAP:
                value_type = "integerValue" if key in INTEGER_KEYS else "doubleValue"
                entries.append({
                    "entryId": f"{module_id}-{key}",
                    "propertyAlias": MODULE_ALIAS_MAP[key],
                    "propertyValues": [{
                        "value": { value_type: int(value) if value_type == "integerValue" else float(value) },
                        "timestamp": { "timeInSeconds": timestamp },
                        "quality": quality
                    }]
                })

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
                    "entryId": f"{module_id}-{sitewise_key}",
                    "propertyAlias": MODULE_ALIAS_MAP[sitewise_key],
                    "propertyValues": [{
                        "value": { "integerValue": int(value) },
                        "timestamp": { "timeInSeconds": timestamp },
                        "quality": quality
                    }]
                })

        # ─── COMPROMISED CELLS ─────────────────────────────────────
        for cell in compromised_cells:
            raw_id = cell.get("id", "")
            parts = raw_id.split("-")
            cell_suffix = parts[-1] if len(parts) == 4 else None

            if not cell_suffix:
                print(f"Skipping invalid cell ID: {raw_id}")
                continue

            asset_name = f"Cell-{cell_suffix.zfill(3)}"  # C004 → Cell-004

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
