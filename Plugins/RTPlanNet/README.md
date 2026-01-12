# RTPlanNet Plugin

## Overview
**RTPlanNet** handles the networking and replication aspects of the application, ensuring the plan state is synchronized across clients.

## Key Functionality
*   **Net Driver**: `ARTPlanNetDriver` is a replicated actor that maintains the authoritative `PlanDocument` on the server.
*   **Replication**: Serializes the plan to JSON (`ReplicatedPlanJson`) for replication to clients.
*   **RPCs**: Provides Server RPCs (`Server_SubmitAddWall`, etc.) for clients to request changes to the plan.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`, `NetCore`
*   **Plugins**: `RTPlanCore`
