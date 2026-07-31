"""Schrijft ALLEEN DT_SquadOrderDefs (SPEC-P1-06 + SPEC-P2-02 Stage B).

Draaien:
  UnrealEditor-Cmd.exe Eclipse.uproject -run=pythonscript
    -script="Eclipse\\Tools\\author_squad_order_lines.py" -unattended -nullrhi

Waarom dit naast create_phase1_content.py bestaat: dat script schrijft de HELE
Phase-1-dataset, inclusief DT_Weapons en DA_CharacterTuning. Zolang er parallel
aan het wapenwerk wordt gesleuteld, zet het draaien daarvan andermans afstelling
terug. Dit script raakt precies een tabel aan.

De regels zelf staan in squad_order_lines.py — dat is de enige bron, en
create_phase1_content.py leest hem ook.
"""
import json
import os
import sys

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from squad_order_lines import SQUAD_ORDER_ROWS  # noqa: E402

DATA_PATH = "/Game/Data"
TABLE_NAME = "DT_SquadOrderDefs"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset = unreal.EditorAssetLibrary

path = f"{DATA_PATH}/{TABLE_NAME}"
if editor_asset.does_asset_exist(path):
    table = editor_asset.load_asset(path)
else:
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.EclipseSquadOrderDefRow.static_struct())
    table = asset_tools.create_asset(TABLE_NAME, DATA_PATH, unreal.DataTable, factory)

if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(
        table, json.dumps(SQUAD_ORDER_ROWS)):
    raise RuntimeError("DT_SquadOrderDefs JSON fill failed")

editor_asset.save_asset(path)

# De koppeling naar DA_SquadTuning alleen ZETTEN als hij ontbreekt. Een tabel die
# al gekoppeld is, opnieuw koppelen zou het tuning-asset vuil maken zonder dat er
# iets verandert — en een vuil asset in een gedeelde werkboom kost iemand anders
# een merge.
tuning_path = f"{DATA_PATH}/DA_SquadTuning"
if editor_asset.does_asset_exist(tuning_path):
    tuning = editor_asset.load_asset(tuning_path)
    if tuning.get_editor_property("order_defs") != table:
        tuning.set_editor_property("order_defs", table)
        editor_asset.save_asset(tuning_path)
        unreal.log("DA_SquadTuning opnieuw gekoppeld aan DT_SquadOrderDefs.")

names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
unreal.log(f"DT_SquadOrderDefs geschreven: {len(names)} rijen.")

# De rijen die de CODE bij naam opzoekt moeten er echt zijn, anders valt de squad
# terug op de stockzin "Copy." en klinkt elke weigering hetzelfde. Luid melden in
# plaats van stil goedkeuren.
required = {row["Name"] for row in SQUAD_ORDER_ROWS}
missing = sorted(required - {str(n) for n in names})
if missing:
    raise RuntimeError(f"DT_SquadOrderDefs mist rijen na het schrijven: {missing}")
unreal.log("Alle order-, reden- en overgangsrijen staan erin.")
