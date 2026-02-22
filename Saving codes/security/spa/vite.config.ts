import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },

  server: {
    proxy: {
      // Toda requisição que o Vue fizer começando com '/api'
      '/api': {
        target: 'http://localhost:8081', // Vai ser redirecionada para o seu BFF
        changeOrigin: true,
        // Remove o '/api' antes de entregar pro Spring
        rewrite: (path) => path.replace(/^\/api/, '') 
      }
    }
  }
})
