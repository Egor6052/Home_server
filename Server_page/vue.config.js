const { defineConfig } = require('@vue/cli-service')

module.exports = defineConfig({
  transpileDependencies: true,
  // Це налаштування критичне для GitHub Pages
  publicPath: process.env.NODE_ENV === 'production'
    ? '/-Home_server_page/'
    : '/'
})