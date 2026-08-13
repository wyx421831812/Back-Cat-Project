<template>
  <section>
    <h2>技术架构</h2>
    <p>使用 <mark class="key">Qt 6 + C++</mark> 开发，架构清晰便于扩展：</p>
    <div class="feature-grid">
      <div class="feature-card" v-for="(item, index) in techItems" :key="index">
        <h4>{{ item.title }}</h4>
        <p>{{ item.description }}</p>
      </div>
    </div>
  </section>
</template>

<script setup>
const techItems = [
  { title: '核心层', description: '窗口管理、透明效果、鼠标事件穿透处理、配置持久化存储' },
  { title: '组件系统', description: '抽象组件基类，所有功能组件基于基类实现，支持热插拔切换' },
  { title: '渲染系统', description: 'QML 或 Widget 渲染，支持抗锯齿圆角、渐变效果流畅动画' },
  { title: '设置界面', description: '原生设置面板，可视化调整所有参数，支持配置导入导出' }
]
</script>

<style scoped>
.feature-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 1.5rem;
  margin-top: 2rem;
}
.feature-card {
  background: var(--bg2);
  padding: 1.5rem;
  border-radius: 12px;
  border-left: 4px solid var(--accent);
  transition: transform 0.3s ease, box-shadow 0.3s ease;
}
.feature-card:hover {
  transform: translateY(-5px);
  box-shadow: var(--shadow);
}
.feature-card h4 {
  font-size: 1.125rem;
  margin-bottom: 0.5rem;
  color: var(--ink);
}
.feature-card p {
  font-size: 0.875rem;
  color: var(--muted);
  margin-bottom: 0;
}

@media (max-width: 768px) {
  .feature-grid { grid-template-columns: 1fr; }
}
</style>