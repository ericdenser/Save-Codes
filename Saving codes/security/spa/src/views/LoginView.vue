<script setup lang="ts">
// A função que dispara o fluxo de login
const fazerLogin = () => {
  // Redireciona o navegador INTEIRO para o Spring Boot (BFF)
  // O Spring vai interceptar isso e mandar para o Keycloak
  window.location.href =  '/api/oauth2/authorization/keycloak'
}
</script>

<template>
  <div class="login-container">
    <div class="login-card">
      <h1>Bem-vindo</h1>
      <p>Prototipo de sistema seguro em desenvolvimento pelo laboratorio Mackleaps</p>
      
      <button @click="fazerLogin" class="btn-login">
        Entrar com Keycloak
      </button>
    </div>
  </div>
</template>

<style scoped>
.login-container {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100vh;
  background-color: #f4f7f6;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
}

.login-card {
  background: white;
  padding: 40px;
  border-radius: 12px;
  box-shadow: 0 10px 25px rgba(0,0,0,0.1);
  text-align: center;
  max-width: 400px;
  width: 100%;
}

h1 {
  color: #2c3e50;
  margin-bottom: 10px;
}

p {
  color: #7f8c8d;
  margin-bottom: 30px;
}

.btn-login {
  background-color: #42b983; /* Verde clássico do Vue */
  color: white;
  border: none;
  padding: 12px 24px;
  font-size: 16px;
  font-weight: bold;
  border-radius: 6px;
  cursor: pointer;
  width: 100%;
  transition: background-color 0.3s ease;
}

.btn-login:hover {
  background-color: #3aa876;
}
</style>