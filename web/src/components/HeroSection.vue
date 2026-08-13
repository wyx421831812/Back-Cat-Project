<template>
  <div class="hero">
    <h1>BackPet</h1>
    <p class="subtitle">Windows 平台可自定义情绪价值桌面组件</p>
    <div>
      <span class="tag">TRAE 创意大赛</span>
      <span class="tag">QT 桌面应用</span>
      <span class="tag">互动宠物</span>
      <span class="tag">情绪价值</span>
      <span class="tag">高度自定义</span>
    </div>
  </div>
</template>

<script setup>
</script>

<style scoped>
.hero {
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(10px);
  border-radius: 20px;
  padding: 3rem 2rem;
  margin: 2rem 0;
  box-shadow: var(--shadow);
  text-align: center;
}
.hero h1 {
  font-size: 3rem;
  font-weight: 800;
  background: linear-gradient(135deg, var(--accent), var(--accent2), var(--accent3));
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  margin-bottom: 1rem;
}
.hero .subtitle {
  font-size: 1.25rem;
  color: var(--muted);
  margin-bottom: 2rem;
}
.tag {
  display: inline-block;
  padding: 0.5rem 1rem;
  background: var(--bg2);
  border-radius: 20px;
  font-size: 0.875rem;
  color: var(--muted);
  margin: 0.25rem;
}

@media (max-width: 768px) {
  .hero h1 { font-size: 2rem; }
}
</style>