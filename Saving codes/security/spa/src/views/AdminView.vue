<script setup lang="ts">
import { ref, onMounted } from 'vue'
import api from '@/services/api'
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()
const mensagemSecreta = ref<string>('')
const erro = ref<string>('')

const carregarDadosAdmin = async () => {
  try {
    const resposta = await api.get('/buscar-tarefas-admin')
    mensagemSecreta.value = resposta.data
  } catch (e: any) {
    erro.value = 'Você foi barrado pelo Backend! Só admins passam daqui.'
  }
}

onMounted(() => {
  carregarDadosAdmin()
})
</script>

<template>
  <div class="admin-container">
    <header class="cabecalho-admin">
      <div class="titulo-area">
        <span class="icone-alerta">⚠️</span>
        <h2>Painel do Administrador</h2>
      </div>
      <router-link to="/tarefas" class="btn-voltar">Voltar para Home</router-link>
    </header>

    <main class="painel-conteudo">
      <div class="bem-vindo">
        <p>Acesso concedido, <strong>{{ authStore.user?.nome }}</strong>.</p>
        <span class="badge-admin">NÍVEL DE ACESSO MÁXIMO</span>
      </div>

      <div class="console-backend">
        <h3>Resposta do Servidor (Porta 8082):</h3>
        
        <div v-if="erro" class="tela-erro">
          {{ erro }}
        </div>
        
        <div v-else-if="!mensagemSecreta" class="tela-loading">
          Estabelecendo conexão segura...
        </div>
        
        <div v-else class="tela-sucesso">
          > {{ mensagemSecreta }}
        </div>
      </div>
    </main>
  </div>
</template>

<style scoped>
.admin-container {
  max-width: 800px;
  margin: 40px auto;
  font-family: 'Courier New', Courier, monospace; /* Fonte estilo terminal */
  background-color: #1e272e;
  color: #d2dae2;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 10px 30px rgba(0,0,0,0.5);
}

.cabecalho-admin {
  background-color: #ff3f34; /* Vermelho alerta */
  color: white;
  padding: 15px 25px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.titulo-area {
  display: flex;
  align-items: center;
  gap: 10px;
}

h2 { margin: 0; font-family: 'Segoe UI', Tahoma, sans-serif; }

.icone-alerta { font-size: 24px; }

.btn-voltar {
  background-color: rgba(255, 255, 255, 0.2);
  color: white;
  text-decoration: none;
  padding: 6px 12px;
  border-radius: 4px;
  font-family: 'Segoe UI', sans-serif;
  font-weight: bold;
  transition: 0.2s;
}

.btn-voltar:hover { background-color: rgba(255, 255, 255, 0.4); }

.painel-conteudo { padding: 30px; }

.bem-vindo {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 30px;
  border-bottom: 1px solid #485460;
  padding-bottom: 15px;
}

.badge-admin {
  background-color: #ff3f34;
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-weight: bold;
  font-size: 12px;
}

.console-backend {
  background-color: #000000;
  border: 1px solid #485460;
  border-radius: 8px;
  padding: 20px;
}

h3 {
  color: #0be881; /* Verde terminal */
  margin-top: 0;
  font-size: 14px;
  border-bottom: 1px dashed #0be881;
  padding-bottom: 5px;
}

.tela-sucesso { color: #0be881; font-size: 16px; margin-top: 15px; }
.tela-erro { color: #ff3f34; font-size: 16px; margin-top: 15px; }
.tela-loading { color: #ffd32a; font-size: 16px; margin-top: 15px; animation: piscar 1s infinite; }

@keyframes piscar {
  0% { opacity: 1; }
  50% { opacity: 0.5; }
  100% { opacity: 1; }
}
</style>