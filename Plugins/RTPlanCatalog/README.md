# RTPlanCatalog Plugin

## Overview
**RTPlanCatalog** manages the library of available products (furniture, materials, doors, windows) that can be placed in the plan.

## Key Functionality
*   **Product Definition**: `FRTProductDefinition` struct defines the properties of a product (ID, Name, Mesh, Dimensions).
*   **Catalog Asset**: `URTProductCatalog` is a Data Asset that holds a collection of product definitions, allowing designers to manage the catalog in the Editor.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`
*   **Plugins**: `RTPlanCore`
