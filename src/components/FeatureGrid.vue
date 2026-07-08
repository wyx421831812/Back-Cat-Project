<template>
  <section>
    <h2>核心特性</h2>
    <div class="feature-grid">
      <div class="feature-card" v-for="(feature, index) in features" :key="index">
        <div class="icon">{{ feature.icon }}</div>
        <h4>{{ feature.title }}</h4>
        <p>{{ feature.description }}</p>
      </div>
    </div>
  </section>
</template>

<script setup>
const features = [
  { icon: '🎨', title: '高度自定义外观', description: '支持自定义颜色主题、宠物、大小位置、自定义背景，用户可以调整透明度让宠物完美融入桌面' },
  { icon: '🧩', title: '多组件切换', description: '类似手机桌面组件，支持多种功能模块自由切换：宠物互动、时钟、天气、待办、寄语等' },
  { icon: '💖', title: '情绪价值互动', description: '点击、拖拽都会触发不同互动反应，定时鼓励语录，时刻给你暖心的情绪支持' },
  { icon: '🖱️', title: '穿透点击', description: '支持点击穿透模式，显示在桌面最上层但不会干扰鼠标操作，不挡事还能随时互动' },
  { icon: '⚡', title: '轻量高效', description: '基于 QT 原生开发，占用内存极低，CPU 使用率几乎可以忽略，长时间运行不卡机' },
  { icon: '📝', title: '每日寄语组件', description: '类似手机的语录组件，每天一句治愈系文字，在你疲惫时给予温暖鼓励' }
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
.feature-card .icon {
  font-size: 2.5rem;
  margin-bottom: 0.75rem;
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