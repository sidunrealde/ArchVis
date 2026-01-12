# RTPlanUI Plugin

## Overview
**RTPlanUI** contains the User Interface widgets (UMG) for the application, including the HUD, toolbars, and property inspectors.

## Key Functionality
*   **UI Base**: `URTPlanUIBase` serves as the root widget, holding references to the Document and Tool Manager.
*   **Toolbar**: `URTPlanToolbar` provides buttons to switch between active tools.
*   **Properties**: `URTPlanProperties` displays and edits properties of selected items (listening for plan changes).
*   **Catalog Browser**: `URTPlanCatalogBrowser` displays the list of available products from the Catalog.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`, `UMG`, `CommonUI`
*   **Plugins**: `RTPlanCore`, `RTPlanTools`, `RTPlanCatalog`
