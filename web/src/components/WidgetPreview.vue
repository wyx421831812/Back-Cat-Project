<template>
  <section>
    <h2>可用组件预览</h2>
    <p>灵感来自手机桌面小组件，用户可以随心切换不同功能：</p>
    <div class="widget-grid">
      <div class="widget-card" v-for="(widget, index) in widgets" :key="index">
        <div class="widget-icon">{{ widget.icon }}</div>
        <h4>{{ widget.title }}</h4>
        <p>{{ widget.description }}</p>
      </div>
    </div>
  </section>
</template>

<script setup>
const widgets = [
  { icon: '🐱', title: '互动宠物', description: 'BongoCat 风格互动' },
  { icon: '🕐', title: '时钟日历', description: '悬浮桌面时钟' },
  { icon: '🌤️', title: '天气挂件', description: '实时天气展示' },
  { icon: '✅', title: '待办清单', description: '极简任务跟踪' },
  { icon: '💬', title: '每日寄语', description: '治愈文字陪伴' },
  { icon: '📊', title: '专注计时', description: '番茄工作法计时' },
  { icon: '🎵', title: '音乐控制', description: '快捷播放控制' },
  { icon: '➕', title: '更多扩展', description: '插件化架构设计' }
]
</script>

<style scoped>
.widget-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 1rem;
  margin-top: 1.5rem;
}
.widget-card {
  background: white;
  padding: 1.5rem;
  border-radius: 12px;
  border: 2px solid var(--rule);
  text-align: center;
  transition: all 0.3s ease;
  cursor: pointer;
}
.widget-card:hover {
  border-color: var(--accent);
  box-shadow: var(--shadow);
}
.widget-card .widget-icon {
  font-size: 3rem;
  margin-bottom: 0.75rem;
}
.widget-card h4 {
  font-size: 1rem;
  margin-bottom: 0.25rem;
}
.widget-card p {
  font-size: 0.8rem;
  color: var(--muted);
  margin-bottom: 0;
}

@media (max-width: 768px) {
  .widget-grid { grid-template-columns: 1fr 1fr; }
}
</style>