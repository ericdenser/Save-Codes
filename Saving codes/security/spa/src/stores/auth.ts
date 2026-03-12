import { defineStore } from 'pinia'
import api from '@/services/api'

export const useAuthStore = defineStore('auth', {
  state: () => ({
    user: null as any,
    isAuthenticated: false,
    isInitialized: false, // Flag crucial para sabermos se já fizemos o F5
  }),
  actions: {
    // 1. A função que checa no BFF (mas só faz isso uma vez!)
    async checkAuth() {
      // Se já fomos no backend nesta sessão do navegador, confia no cache e sai da função!
      if (this.isInitialized) return

      try {
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
        this.isInitialized = true // Marca que a checagem inicial foi feita
      }
    },

    // 2. Limpa tudo (usado no logout ou quando a sessão expira)
    clearAuth() {
      this.user = null
      this.isAuthenticated = false
      this.isInitialized = true
    }
  }
})
