
# Protecting the SPA (Vue.js) with the BFF - Part 1
![Vue.js](https://img.shields.io/badge/Vue.js-3.0-42b983?style=flat-square&logo=vue.js)



In this tutorial, we will build the Presentation Layer of our architecture. 

You will learn how to connect a Single Page Application (SPA) built with Vue.js to our Spring Boot BFF securely.


> [!Important] This guide continues from the previous ArchitectureBackendForFrontend tutorial. 
> Before proceding with the paths below, make sure you followed all the steps presented on the last tutorials **([BFF](LearningPaths/ArchitectureBFFandKeycloak/ProtectingTheApi_BFF.md), [Api_Authentication](LearningPaths/ArchitectureBFFandKeycloak/ProtectingApi_Authentication.md), [Api_Authorization](LearningPaths/ArchitectureBFFandKeycloak/ProtectingApi_Authorization.md))**

> [!Note] Future Implementation
> To keep this first contact simple, we are assuming your BFF currently has CSRF disabled. In the next tutorial (Part 2), we will harden this application by enabling CSRF, handling error codes, and adding Role-based UI constraints.

## Prerequisites

1. VsCode installed ([Windows tutorial](ide/InstallVSCodeInWindows.md), [Debian tutorial](ide/InstallVSCode-Debian.md))
2. **Node.js** installed on your machine.
3. The **BFF (Port 8081)** and **Resource Server API (Port 8082)** running from the previous tutorials.
4. **Keycloak** running on Docker.

---

## Project Initialization

First, let's create a blank Vue project configured with the necessary tools.

1. Open your terminal and run the following command:

```bash
npm create vue@latest

```

2. When prompted, select the following options:
* **Project name:** `frontend-spa`
* **Add TypeScript?** Yes
* **Add JSX Support?** No
* **Add Vue Router for Single Page Application development?** Yes
* **Add Pinia for state management?** Yes
* **Add ESLint/Prettier?** Yes (Optional, but recommended)


3. Enter the project folder, install the dependencies, and install `axios` (our HTTP client):

```bash
cd frontend-spa
npm install
npm install axios

```

---

## The Vite Proxy (Solving CORS)

During development, your Vue app runs on a different port than the bff's, for example SPA on `localhost:5173`, but your BFF runs on `localhost:8081`. Browsers block requests between different ports due to **CORS (Cross-Origin Resource Sharing)**.

Instead of weakening our backend security to accept cross-origin requests, we will use Vite to proxy our API calls.

Open `vite.config.ts` and add the `server` block:

```typescript
import { fileURLToPath, URL } from 'node:url'
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
  // NEW CONFIGURATION BELOW
  server: {
    proxy: {
      // Any request starting with '/api' will be forwarded to the BFF
      '/api': {
        target: 'http://localhost:8081', 
        changeOrigin: true,
        // Removes the '/api' prefix before delivering to Spring
        rewrite: (path) => path.replace(/^\/api/, ''),
      },
    },
  },
})

```

> [!Note] How it works
> If Axios calls `http://localhost:5173/api/me`, Vite intercepts it and automatically redirects from the frontend to a separate backend server. This allows our frontend to communicate with the backend seamlessly while avoiding common CORS issues.

---

## Axios Configuration (The Secure Communicator)

Axios is a library used to make HTTP requests to our backend. We use it instead of the native browser fetch because it allows us to set global default rules for every request and handle our security architecture.

Create a file named `api.ts` inside `src/services/` (create the folder if it doesn't exist):

```typescript
import axios from 'axios'

const api = axios.create({
  baseURL: '/api', // This triggers the Vite proxy we just configured
  
  // CRITICAL: Tells Axios to include the SESSION cookie in every request
  withCredentials: true 
})

export default api
```

---

## State Management with Pinia

Pinia is the official state management library for Vue. Think of it as the "global memory" of your frontend.

When a user logs in, we need to remember their name and the fact that they are authenticated across all different pages of our application. Instead of asking the backend "Am I logged in?" every time the user clicks a button, we ask once and save the answer in Pinia.

Open `src/stores/auth.ts` (replace the default counter store) and implement our Auth Store:

```typescript
import { defineStore } from 'pinia'
import api from '@/services/api'

export const useAuthStore = defineStore('auth', {
  // 1. STATE: The variables we want to remember
  state: () => ({
    user: null as any,
    isAuthenticated: false,
    isInitialized: false, // Prevents asking the backend multiple times
  }),
  
  // 2. ACTIONS: The functions that change the state
  actions: {
    async checkAuth() {
      // If we already checked during this page load, trust the memory and stop.
      if (this.isInitialized) return

      try {
        // Asks the BFF: "Is my cookie valid?"
        const resposta = await api.get('/me') 
        this.isAuthenticated = resposta.data.authenticated
        
        if (this.isAuthenticated) {
          this.user = { 
            nome: resposta.data.nome, 
            username: resposta.data.username
          }
        }
      } catch (error) {
        this.isAuthenticated = false
      } finally {
        this.isInitialized = true 
      }
    },

    // Cleans user (for logout and session expiration)
    clearAuth() {
      this.user = null
      this.isAuthenticated = false
      this.isInitialized = true
    }
  }
})
```

---

## Route Guards (Protecting the UI)

Vue Router controls the navigation between different pages in our SPA.

A Navigation Guard is a security checkpoint. Even though our real security is in the backend, we must prevent users from navigating to restricted pages in the frontend interface if they are not logged in.

Open `src/router/index.ts` and set up the routes and the security guard:

```typescript

import { createRouter, createWebHistory } from 'vue-router'
import LoginView from '../views/LoginView.vue'
import { useAuthStore } from '@/stores/auth'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    { path: '/', name: 'login', component: LoginView },
    { path: '/home', name: 'home', component: () => import('../views/HomeView.vue') }
  ]
})

// THE SECURITY GUARD (Runs before every page change)
router.beforeEach(async (to, from, next) => {
  const authStore = useAuthStore()
  
  // Forces Pinia to check the session status with the BFF
  await authStore.checkAuth() 

  // Not logged in and trying to access a secure page? Send to Login.
  if (to.name !== 'login' && !authStore.isAuthenticated) {
    return next({ name: 'login' })
  } 
  
  // Logged in and trying to access the Login page? Send to Home.
  if (to.name === 'login' && authStore.isAuthenticated) {
    return next({ name: 'home' })
  } 

  // Allow navigation
  next()
})

export default router
```

---

## Triggering the Login and Logout

Because we are using an SSO (Single Sign-On) architecture, the Login and Logout processes are slightly different from traditional JWT apps.

### The Login View (Public area)

Create `src/views/LoginView.vue`. 

```vue
<script setup lang="ts">
const doLogin = () => {
  // Redirects the entire browser to the Spring Boot (BFF)
  // Spring will intercept this and forward the user to Keycloak
  window.location.href = '/api/oauth2/authorization/keycloak'
}
</script>

<template>
  <div class="login-container">
    <button @click="doLogin" class="btn-login">Login with Keycloak</button>
  </div>
</template>
```


### The Home View (Protected Area)

Create `src/views/HomeView.vue`. Here we will use our configured Axios (api.ts) to fetch data from the backend.

```vue
<script setup lang="ts">
import { ref, onMounted } from 'vue'
import api from '@/services/api' 
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()
const avatars = ref<any[]>([]) 
const error = ref<string>('')

// Fetches data when the component loads
const loadAvatars = async () => {
  try {
    const response = await api.get('/avatar') 
    avatars.value = response.data
  } catch (e) {
    error.value = 'Failed to load avatars.'
    console.error(e)
  }
}

onMounted(() => {
  loadAvatars()
})
</script>

<template>
  <div class="home-container">
    <h2>Hello, {{ authStore.user?.nome }}!</h2>
    <p>Welcome to your protected area.</p>
    
    <div v-if="error" style="color: red;">{{ error }}</div>
    
  </div>
</template>
```


### The Logout 

When logging out, we must send a `POST` request to `/logout`.

Add this method to your `HomeView.vue` (or wherever your logout button is):


```typescript
<script setup lang="ts">
import { ref, onMounted } from 'vue'
import api from '@/services/api' 
import { useAuthStore } from '@/stores/auth'

// ... other methods

// OUR NEW LOGOUT METHOD
const doLogout = () => {
  // Create an invisible HTML form dynamically
  const form = document.createElement('form')
  form.method = 'POST'
  form.action = '/api/logout' // Proxied to the BFF

  // Append to the document and submit
  document.body.appendChild(form)
  form.submit() 
}

onMounted(() => {
  carregarTarefas()
})
</script>

<template>
  <div class="home-container">
    <h2>Hello, {{ authStore.user?.nome }}!</h2>
    <p>Welcome to your protected area.</p>

     // Our new Button to logout
    <button @click="doLogout" class="btn-sair">Sair do Sistema</button>
    
    <div v-if="error" style="color: red;">{{ erro }}</div>
    
  </div>
</template>
```

---

## Running the Application

1. Start your Vue application:

```bash
npm run dev

```

2. Navigate to `http://localhost:5173`.
3. Click "Login" and join with any User we created last Tutorial (ADMIN OR USER). You should be redirected to Keycloak.
4. After logging in, Keycloak will send you back to the BFF, which will establish the secure Redis session and drop you perfectly into your Vue interface!

### Next tutorial

Congratulations! You have successfully implemented a state-of-the-art, secure SPA architecture using the Token Handler (BFF) pattern. We strongly recommend you to continue to this next tutorial [ProtectingSpa_Advanced.md]
