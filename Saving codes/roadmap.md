# Roadmap: BFF Architecture & API Security
![Vue.js](https://img.shields.io/badge/Vue.js-3.0-42b983?style=flat-square&logo=vue.js)
![Spring Boot](https://img.shields.io/badge/Spring_Boot-3.x-6DB33F?style=flat-square&logo=spring)
![Keycloak](https://img.shields.io/badge/Keycloak-OIDC-0097CE?style=flat-square&logo=keycloak)
![Redis](https://img.shields.io/badge/Redis-Session-DC382D?style=flat-square&logo=redis)


Welcome to the learning path for modern application security. In this module, you will learn how to build and implement a professional, distributed architecture using **Spring Boot**, **Keycloak**, and **Redis**.

![Image01](LearningPaths/images/roadmap.png)
---

### The Learning Journey

To master these concepts, follow the documentation in this specific order:


#### 1. The Architectural Foundation

This is the theoretical understanding of the ecosystem. Do not skip this section, as it holds the concepts you will apply in practice.

* **[ExplaningArchictureBFF.md](LearningPaths/ArchitectureBFFandKeycloak/ExplaningArchicture.md)**: An introduction to the **BFF (Backend For Frontend)** pattern. This file covers the difference between **Stateless** and **Stateful** systems and explains why microservices require this specialized layer.

#### 2. BFF Implementation

It is time to build the infrastructure and the code.

* **[ProtectingTheAPI_BFF.md](LearningPaths/ArchitectureBFFandKeycloak/ProtectingTheApi_BFF.md)**: The core practical tutorial. You will configure **Docker** (Keycloak, Postgres, and Redis) and build a Spring Boot application using `SecurityConfig` and a protected `BffController`.

#### 3. Understanding the Security Protocols

Learn the implementation of industry-standard security protocols to manage identity verification and resource protection across distributed systems.

* **[ProtectingApi_Authentication.md](LearningPaths/ArchitectureBFFandKeycloak/ProtectingApi_Authentication.md)**: Focused on **OIDC (OpenID Connect)**. Learn how the system verifies "who" the user is (Identity).
* **[ProtectingApi_Authorization.md](LearningPaths/ArchitectureBFFandKeycloak/ProtectingApi_Authorization.md )**: Focused on **OAuth 2.0**. Learn how the system defines "what" the user is allowed to do (Permissions/Scopes).



#### 4. Frontend Integration

How to apply these backend protections to modern client-side applications.

* **[ProtectingSpa.md](LearningPaths/ArchitectureBFFandKeycloak/ProtectingSpa.md)**: Learn how to connect a **Single Page Application (SPA)** to your BFF while ensuring that sensitive security tokens never leave the server.

#### 5. Advanced Frontend Integration

How to protect your SPA with advanced security strategies and enable Role-Based views.

* **[ProtectingSpa_Advanced.md](LearningPaths/ArchitectureBFFandKeycloak/ProtectingSpa_Advanced.md)**: Learn how to enable CSRF, handle error codes, add Role-based UI constraints and strengthen your system with HTTP Security Headers.
---

### Key Technologies You Will Master

* **Architecture:** BFF (Backend For Frontend) & Microservices.
* **Identity & Auth:** Keycloak (OIDC & OAuth 2.0).
* **Infrastructure:** Docker & Docker Compose.
* **Session Management:** Redis (High-speed In-memory Database).


> [!TIP]
> **Pro-Tip for Students:**
> As you read, focus on how the **BFF** acts as a "security vault." Its main job is to keep complex tokens safe in **Redis** while giving the browser a simple, safe session cookie.



