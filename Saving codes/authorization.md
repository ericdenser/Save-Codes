# Protecting the API with Authorization

In this tutorial, we will teach you how to protect your API with Authorization.

> [!Important] This guide continues from a previous tutorial. 
> We use the same project we finished from last tutorial. If your application is missing something, your tests might fail. So before proceding with the implementations, make sure you followed all the steps presented on the [Protecting your API with Authentication](LearningPaths/ArchitectureBFFandKeycloak/ProtectingApi_Authentication.md). 

# Prerequisites

1. VsCode installed ([Windows tutorial](ide/InstallVSCodeInWindows.md), [Debian tutorial](ide/InstallVSCode-Debian.md))
2. Java and Maven configured in your development environment. [Tutorials here](https://mackcloud.mackenzie.br/gitlab/fci/learning/-/tree/main/java?ref_type=heads)
3. API implemented with simple controller (This tutorial will use the same API developed on the [MyFirstJavaRestAPI tutorial](https://mackcloud.mackenzie.br/gitlab/fci/learning/-/blob/main/LearningPaths/MyFirstJavaRestAPI.md?ref_type=heads))
4. BFF implemented with Keycloak and Redis [Tutorial]()
5. API implemented with Authentication [Tutorial]()
--- 

# Protecting the API

## Authorization explaining

In the last tutorial, you learned and implemented Authentication. While Authentication is about verifying who you are, Authorization is about determining what you are allowed to do.

Authorization is one of the most important safety layers on any application. You can block, allow and limit what the user access. Basically every user will have a ROLE, and that ROLE is used on every request that the API can verify the user's permissions. Based on your business logic, the server decides whether to grant access to the requested resource or block it.

## Role-Based Access Control (RBAC)

Managing permissions user by user is impossible to maintain. To resolve this, we use RBAC (Role-Based Access Control). Instead of assigning permissions directly to individuals, we assign them to Roles, and then assign those roles to users.

---

## Implementation

There are many ways of implementing Authorization on your system. Spring Boot offers quick and easy tools to activate Authorization, that we only use on the first test to verify if the ROLE is being assigned. For the final implementation, we present you a more flexible and practical strategy than the first one.

Lest start.

### Creating Role on Keycloak

1. Open the Keycloak admin interface.
2. Select the Realm of your application.
3. On the left navigation bar, click "Realm Roles".

It will lead you to this page:

![Image01](LearningPaths/images/_protectingApi__keycloak_roles.png)

4. Click on "Create Role"
5. It will open a window asking the "Role Name" and "Description", name it as ADMIN 
6. Click on save and the Role is successfully created.
7. Create another Role named USER.

### Assigning the Role to a User

1. Still on the Keycloak admin interface, on the left navigation bar, click on "Users".
2. It will open a window of all the created Users on your realm. 
3. Select a user to assign the Role (Use an existing one or Create a new User)
4. Click on Role Mapping -> Assign Role 

It opens this page, click on "Filter by clients" and change for "Filter by realm role"

![Image02](LearningPaths/images/_protectingApi__keycloak_assign_role.png)

5. It will open a list of roles, select ADMIN -> Assign.
6. Do the same steps again and assign the role USER to another user (Create a new one if needed)

### Applying the Role on Spring

Now that we have a Role created and assigned to a User, we need to apply the Authorization strategy on the API. Before you write the code, you need to understand a small conflict between Keycloak and Spring Security.

Keycloak stores our roles inside Access Token. If you inspect the token on jwt.io, you will find this:

```json
  "realm_access": {

    "roles": [

      "default-roles-bff-spring",

      "offline_access",

      "USER",

      "uma_authorization",

      "ADMIN"

    ]

  },
```

We have to teach Spring how to extract this specific data inside the JWT and convert it into standard Spring Authorities (Roles).

### Creating the Role Converter

To fix this, we created a custom converter. Create a new class called `KeycloakRoleConverte` inside config folder of your API.

```java
package com.example.resource_server.config;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.oauth2.server.resource.authentication.JwtAuthenticationConverter;

@Configuration
public class RoleCatcher {

    @Bean
    public JwtAuthenticationConverter conversorDeRolesDoKeycloak() {
        JwtAuthenticationConverter converter = new JwtAuthenticationConverter();
        
        converter.setJwtGrantedAuthoritiesConverter(jwt -> {
            // map realm_acces
            Map<String, Object> realmAccess = jwt.getClaim("realm_access");
            
            // if its null or doesnt contain any role
            if (realmAccess == null || !realmAccess.containsKey("roles")) {
                return List.of(); // return empty
            }
            
            // Extracts the Role list
            List<String> rolesDoKeycloak = (List<String>) realmAccess.get("roles");
            
            // Map each role to the current user with the prefix "ROLE_" 
            return rolesDoKeycloak.stream()
                    .map(roleName -> new SimpleGrantedAuthority("ROLE_" + roleName.toUpperCase()))
                    .collect(Collectors.toList());
        });
        
        return converter;
    }
}
```

Lets apply it on our FilterChain.  
Open your `SecurityConfig` class and make the same changes below:

```java
package com.example.resource_server.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.security.config.annotation.method.configuration.EnableMethodSecurity;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.http.SessionCreationPolicy;
import org.springframework.security.oauth2.jwt.*;
import org.springframework.security.web.SecurityFilterChain;

@Configuration
@EnableWebSecurity 
@EnableMethodSecurity // This allows us to use authorization annotations directly on our controllers
public class SecurityConfig {

    @Value("${spring.security.oauth2.resourceserver.jwt.issuer-uri}")
    private String issuerUri;

    @Bean
    public SecurityFilterChain filterChain(HttpSecurity http, JwtDecoder jwtDecoder, JwtAuthenticationConverter roleCatcher) throws Exception {
        http
            .csrf(csrf -> csrf.disable()) 
            
            .sessionManagement(session -> session.sessionCreationPolicy(SessionCreationPolicy.STATELESS)) 
            .authorizeHttpRequests(authorize -> authorize.anyRequest().authenticated()) // Any request must be authenticated

            .oauth2ResourceServer(oauth2 -> oauth2
                .jwt(jwt -> jwt
                    .decoder(jwtDecoder) // our customized decoder to check token type
                    .jwtAuthenticationConverter(roleCatcher)  // OUR NEW CLASS 
                ) 
            );

        return http.build();
    }

}
```
Our API is now reading the ROLE from JWT and linking to the current User. Lets test it blocking some endpoints. 

### Testing 

Create 2 new methods inside your API Controller, one will be avaiable for your role, and the other dont. 
We use the @PreAuthorize annotation to define exactly which roles can access specific methods.

```java
    // Allowed for USERS and ADMINS
    @GetMapping("/avatar")
    @PreAuthorize("hasAnyRole('USER', 'ADMIN')")
    public List<String> listAvatar() {
        return List.of("Obi wan", "Padawan", "Darth Vader");
    }

    // Allowed only for ADMIN 
    @GetMapping("/admin")
    @PreAuthorize("hasRole('ADMIN')") 
    public String adminPage() {
        return "You have ADMIN role, welcome sir!";
    }
```

[!Note] ROLE_ Prefix 
Spring automatically adds the ROLE_ prefix when evaluating hasRole. Thats why we needed our converter successfully mapping the Keycloak "ADMIN" to "ROLE_ADMIN", so the match will work perfectly.

#### Testing on ThunderClient
- Testing Unauthorized requests:

1. Run your BFF and make login with the Keycloak user that you assigned with the role **USER**. 
2. Open ThunderClient on VsCode (Command Palette -> ThunderClinet new Request)
3. Set the request as a GET method to the /avatar endpoint on your API.
4. Attach the Access Token on Auth -> Bearer and paste it there (You can get it with the /compareTokens controller we created last tutorial)

Expected Result:
![Image03](protectingApi_AuthorizationUSER200)

5. Change the route from /avatar to /admin

Expected Result:
![Image04](protectingApi_AuthorizationUSER403)

- Testing Authorized requests:

1. Run the same steps above but now login with the user you assigned with the role **ADMIN**.
2. Get the Access Token of the new user, attach on ThunderClient.
3. Try /admin route

Expected Result:
![Image05](protectingApi_AuthorizationADMIN200)

#### Testing BFF x API

Run both your BFF and API applications. 
> ![Note] Check your BFF Controller
> Make sure you have these 2 controllers on your BFF, 1 requesting to /avatar and other to /admin

- Testing Authorized requests:
1. Open your browser and navigate to your BFF route that calls the API's /admin.
2. Login using the Keycloak User to which you assigned the ADMIN role.
3. Result: You should see the message:  "You have ADMIN role, welcome sir!" (HTTP 200 OK).

- Testing Unauthorized requests:
1. Log out.
2. Login using a different Keycloak User (one that does NOT have the ADMIN role).
3. Try to access the same endpoint.
4. Result: The system will reject the request. You will receive a 403 Forbidden error.

**Congratulations! Your API is now fully protected with robust, scalable Role-Based Access Control integrated perfectly with Keycloak.**

---
### Next tutorial

You finished the "Protecting Your API with Authentication tutorial", we strongly recommend you to do this next tutorial: [Protecting Your Api with Authorization]()







