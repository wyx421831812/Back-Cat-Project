#include "bongomodelmanager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QRandomGenerator>
#include <QStandardPaths>

// 前置声明：静态函数，在 loadModels() 中被调用
static QList<BongoModel> loadBongoCatPresetModels();

// 获取默认模型路径（按优先级返回第一个存在的目录）
// 1. 项目内 resources/models（随程序部署，推荐）
// 2. BongoCat 官方安装目录（用户机器上已安装 BongoCat 时）
static QString getDefaultModelsPath()
{
    // 1. 优先：可执行文件同级目录下的 resources/models（部署目录）
    QString appDir = QCoreApplication::applicationDirPath();
    QString appLocal = appDir + "/resources/models";
    if (QFile::exists(appLocal + "/standard/cat.model3.json")) {
        return appLocal;
    }

    // 2. 开发/调试阶段：向上回溯查找源码目录 desktop/resources/models
    //    (Qt Creator 影子构建: <proj>/desktop/build/<kit>/bin -> ../../../resources/models)
    //    避免在装了 BongoCat 的机器上误用官方安装目录导致模型来源不一致
    QString probe = appDir;
    for (int i = 0; i < 3; ++i) {
        probe += "/..";
        QString devLocal = QDir(probe + "/resources/models").absolutePath();
        if (QFile::exists(devLocal + "/standard/cat.model3.json")) {
            return devLocal;
        }
    }

    // 3. 回退：BongoCat 官方安装目录
#ifdef Q_OS_WIN
    return QStringLiteral("C:/Program Files/BongoCat/assets/models");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("/Applications/BongoCat.app/Contents/Resources/assets/models");
#else
    return QStringLiteral("/usr/local/share/BongoCat/assets/models");
#endif
}

BongoModelManager &BongoModelManager::instance()
{
    static BongoModelManager s_instance;
    return s_instance;
}

BongoModelManager::BongoModelManager(QObject *parent)
    : QObject(parent)
{
    ensureDirectories();
    m_currentModelId = loadCurrentModelId();
}

QStringList BongoModelManager::supportedKeys()
{
    return QStringList{
        "KeyA", "KeyB", "KeyC", "KeyD", "KeyE", "KeyF", "KeyG",
        "KeyH", "KeyI", "KeyJ", "KeyK", "KeyL", "KeyM", "KeyN",
        "KeyO", "KeyP", "KeyQ", "KeyR", "KeyS", "KeyT",
        "KeyU", "KeyV", "KeyW", "KeyX", "KeyY", "KeyZ",
        "Num1", "Num2", "Num3", "Num4", "Num5",
        "Num6", "Num7", "Num8", "Num9", "Num0",
        "Space", "Return", "Shift", "Control", "Alt",
        "Tab", "Backspace", "Escape", "CapsLock",
        "Left", "Right", "Up", "Down"
    };
}

void BongoModelManager::ensureDirectories()
{
    QDir dir;
    dir.mkpath(userModelsDirectory());
}

QString BongoModelManager::userModelsDirectory() const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataPath + "/bongo_models";
}

void BongoModelManager::loadModels()
{
    m_models.clear();

    // 1. 加载经典小键盘预设 (默认)
    BongoModel presetDefault = loadPresetFromResources();
    if (presetDefault.isValid()) {
        m_models[presetDefault.id] = presetDefault;
    }

    // 2. 加载 BongoCat 官方 3 套预设模型 (standard/keyboard/gamepad)
    QList<BongoModel> bongoPresets = loadBongoCatPresetModels();
    for (const BongoModel &m : bongoPresets) {
        if (m.isValid() || !m.live2dModelFile.isEmpty()) {
            m_models[m.id] = m;
        }
    }

    // 3. 加载用户自定义模型
    loadUserModels();

    // 如果当前模型无效，优先使用 BongoCat standard (Live2D)
    if (!m_models.contains(m_currentModelId) && !m_models.isEmpty()) {
        if (m_models.contains("preset_bongocat_standard")) {
            m_currentModelId = "preset_bongocat_standard";
        } else if (m_models.contains("preset_default_cat")) {
            m_currentModelId = "preset_default_cat";
        } else {
            m_currentModelId = m_models.firstKey();
        }
    }

    emit modelsLoaded();
}

BongoModel BongoModelManager::loadPresetFromResources()
{
    BongoModel model;
    model.id = "preset_default_cat";
    model.name = QStringLiteral("经典小键盘 (默认)");
    model.isPreset = true;
    model.mode = "standard";

    // 从Qt资源加载经典小键盘模型图片
    model.coverImage = QPixmap(":/assets/bongo/cover.png");
    model.backgroundImage = QPixmap(":/assets/bongo/background.png");

    // 加载按键图片 (经典小键盘标准模式支持的按键)
    static const QMap<QString, QString> keyMap = {
        {"KeyA", "KeyA"}, {"KeyD", "KeyD"}, {"KeyE", "KeyE"},
        {"KeyQ", "KeyQ"}, {"KeyR", "KeyR"}, {"KeyS", "KeyS"}, {"KeyW", "KeyW"},
        {"Num1", "Num1"}, {"Num2", "Num2"}, {"Num3", "Num3"},
        {"Num4", "Num4"}, {"Num5", "Num5"}, {"Num6", "Num6"}, {"Num7", "Num7"},
        {"Space", "Space"}
    };

    for (auto it = keyMap.begin(); it != keyMap.end(); ++it) {
        QString path = QString(":/assets/bongo/keys/%1.png").arg(it.value());
        QPixmap pix(path);
        if (!pix.isNull()) {
            model.keyImages[it.key()] = pix;
        }
    }

    return model;
}

// 加载 BongoCat 官方 3 套预设模型 (standard/keyboard/gamepad)
// 加载优先级：
//   1. 项目内 resources/models（部署目录或源码目录，由 getDefaultModelsPath() 决定）
//   2. BongoCat 官方安装目录 (C:\Program Files\BongoCat\assets\models)
//   3. Qt 资源 (:/resources/models/) 作为最终回退
static QList<BongoModel> loadBongoCatPresetModels()
{
    QList<BongoModel> result;
    const QVector<QPair<QString, QString>> modeList = {
        {"standard", QStringLiteral("BongoCat · 标准模式")},
        {"keyboard", QStringLiteral("BongoCat · 键盘模式")},
        {"gamepad",  QStringLiteral("BongoCat · 手柄模式")}
    };

    // === 优先从 BongoCat 官方安装目录加载 (文件系统路径，Live2D 可直接访问) ===
    QString defaultPath = getDefaultModelsPath();
    bool defaultPathAvailable = QFile::exists(defaultPath);
    if (defaultPathAvailable) {
        qDebug() << "[BongoModelManager] found default models path:" << defaultPath;
    }

    for (const auto &item : modeList) {
        const QString &mode = item.first;
        const QString &displayName = item.second;

        // 决定模型目录: 优先用默认路径，否则用 Qt 资源
        QString modelDir;
        bool fromFileSystem = false;
        if (defaultPathAvailable) {
            QString fsDir = defaultPath + "/" + mode;
            if (QFile::exists(fsDir + "/cat.model3.json")) {
                modelDir = fsDir;
                fromFileSystem = true;
            }
        }
        if (modelDir.isEmpty()) {
            modelDir = QString(":/resources/models/%1").arg(mode);
        }

        QString model3Path = modelDir + "/cat.model3.json";
        if (!QFile::exists(model3Path)) {
            qWarning() << "[BongoModelManager] preset missing:" << model3Path;
            continue;
        }

        BongoModel m;
        m.id = "preset_bongocat_" + mode;
        m.name = displayName;
        m.mode = mode;
        m.isPreset = true;
        // path 保存模型目录 (用于 BongoCatWidget 拼 model3 路径传给 Live2DWidget)
        m.path = modelDir;
        m.live2dModelFile = model3Path;

        // 加载 resources/ 下的图片
        QString resourcesDir = modelDir + "/resources";
        QString coverPath = resourcesDir + "/cover.png";
        if (QFile::exists(coverPath)) m.coverImage = QPixmap(coverPath);
        QString bgPath = resourcesDir + "/background.png";
        if (QFile::exists(bgPath)) m.backgroundImage = QPixmap(bgPath);

        // 加载 left-keys 和 right-keys 按键图
        static const QStringList keyDirs = {"left-keys", "right-keys", "keys"};
        for (const auto &kd : keyDirs) {
            QString keysDirPath = resourcesDir + "/" + kd;

            if (fromFileSystem) {
                // 文件系统路径: 用 QDir 枚举所有按键图 (更灵活)
                QDir keysDir(keysDirPath);
                if (!keysDir.exists()) continue;
                QStringList filters;
                filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
                QStringList keyFiles = keysDir.entryList(filters, QDir::Files);
                for (const auto &kf : keyFiles) {
                    QString keyName = QFileInfo(kf).baseName();
                    QPixmap pix(keysDirPath + "/" + kf);
                    if (!pix.isNull()) m.keyImages[keyName] = pix;
                }
            } else {
                // Qt 资源路径: qrc 不能枚举目录，使用硬编码按键名列表
                static const QStringList leftKeys = {
                    "Alt","AltGr","BackQuote","Backspace","CapsLock","Control","ControlLeft","ControlRight",
                    "Delete","Escape","Fn","KeyA","KeyB","KeyC","KeyD","KeyE","KeyF","KeyG","KeyH","KeyI",
                    "KeyJ","KeyK","KeyL","KeyM","KeyN","KeyO","KeyP","KeyQ","KeyR","KeyS","KeyT","KeyU",
                    "KeyV","KeyW","KeyX","KeyY","KeyZ","Meta","Num0","Num1","Num2","Num3","Num4","Num5",
                    "Num6","Num7","Num8","Num9","Return","Shift","ShiftLeft","ShiftRight","Slash","Space","Tab",
                    // gamepad left
                    "DPadDown","DPadLeft","DPadRight","DPadUp","LeftTrigger","LeftTrigger2"
                };
                static const QStringList rightKeys = {
                    "DownArrow","LeftArrow","RightArrow","UpArrow",
                    // gamepad right
                    "East","North","RightTrigger","RightTrigger2","South","West"
                };
                for (const auto &kn : leftKeys) {
                    QString p = keysDirPath + "/" + kn + ".png";
                    if (QFile::exists(p)) {
                        QPixmap pix(p);
                        if (!pix.isNull()) m.keyImages[kn] = pix;
                    }
                }
                for (const auto &kn : rightKeys) {
                    QString p = keysDirPath + "/" + kn + ".png";
                    if (QFile::exists(p)) {
                        QPixmap pix(p);
                        if (!pix.isNull()) m.keyImages[kn] = pix;
                    }
                }
            }
        }

        if (m.isValid() || !m.live2dModelFile.isEmpty()) {
            if (fromFileSystem) {
                qDebug() << "[BongoModelManager] loaded preset from default path:" << modelDir;
            }
            result.append(m);
        }
    }

    return result;
}

void BongoModelManager::loadUserModels()
{
    QDir userDir(userModelsDirectory());
    if (!userDir.exists()) return;

    QStringList subDirs = userDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDir : subDirs) {
        QString dirPath = userModelsDirectory() + "/" + subDir;
        BongoModel model = autoDetectAndLoadModel(dirPath, false);
        if (model.isValid()) {
            m_models[model.id] = model;
        }
    }
}

BongoModel BongoModelManager::autoDetectAndLoadModel(const QString &dirPath, bool isPreset)
{
    Q_UNUSED(isPreset)

    // 检测是否为BongoCat标准格式 (有cat.model3.json)
    QString model3Path = dirPath + "/cat.model3.json";
    if (QFile::exists(model3Path)) {
        return loadBongoCatFormat(dirPath);
    }

    // 检测是否有resources目录
    if (QDir(dirPath + "/resources").exists()) {
        return loadBongoCatFormat(dirPath);
    }

    // 否则尝试简化格式
    return loadSimpleFormat(dirPath);
}

BongoModel BongoModelManager::loadBongoCatFormat(const QString &dirPath)
{
    BongoModel model;
    model.path = dirPath;

    // 尝试读取model.json或从目录名获取名称
    QString configPath = dirPath + "/model.json";
    if (QFile::exists(configPath)) {
        QFile file(configPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            QJsonObject obj = doc.object();
            model.id = obj.value("id").toString();
            model.name = obj.value("name").toString();
        }
    }

    if (model.id.isEmpty()) {
        model.id = "user_" + QFileInfo(dirPath).fileName();
    }
    if (model.name.isEmpty()) {
        model.name = QFileInfo(dirPath).fileName();
    }

    // 加载resources目录下的图片
    QString resourcesDir = dirPath + "/resources";

    // cover.png - 猫咪底图
    QString coverPath = resourcesDir + "/cover.png";
    if (QFile::exists(coverPath)) {
        model.coverImage = QPixmap(coverPath);
    }

    // background.png - 背景图
    QString bgPath = resourcesDir + "/background.png";
    if (QFile::exists(bgPath)) {
        model.backgroundImage = QPixmap(bgPath);
    }

    // left_hand.png / right_hand.png - 手部按下图片
    QString leftHandPath = resourcesDir + "/left_hand.png";
    if (QFile::exists(leftHandPath)) {
        model.leftHandDownImage = QPixmap(leftHandPath);
    }

    QString rightHandPath = resourcesDir + "/right_hand.png";
    if (QFile::exists(rightHandPath)) {
        model.rightHandDownImage = QPixmap(rightHandPath);
    }

    // 加载按键图片 - left-keys 目录
    QString leftKeysDir = resourcesDir + "/left-keys";
    if (QDir(leftKeysDir).exists()) {
        loadKeyImages(leftKeysDir, model);
    }

    // 也检查 resources/keys 目录 (兼容旧格式)
    QString keysDir = resourcesDir + "/keys";
    if (QDir(keysDir).exists()) {
        loadKeyImages(keysDir, model);
    }

    // 检查根目录下的keys目录
    QString rootKeysDir = dirPath + "/keys";
    if (QDir(rootKeysDir).exists()) {
        loadKeyImages(rootKeysDir, model);
    }

    // 加载Live2D模型信息
    QString model3Path = dirPath + "/cat.model3.json";
    if (QFile::exists(model3Path)) {
        model.live2dModelFile = model3Path;

        // 尝试读取模型配置获取纹理路径
        QFile modelFile(model3Path);
        if (modelFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(modelFile.readAll());
            modelFile.close();
            QJsonObject obj = doc.object();
            QJsonObject refs = obj.value("FileReferences").toObject();
            QJsonArray textures = refs.value("Textures").toArray();
            if (!textures.isEmpty()) {
                model.live2dTextureFile = dirPath + "/" + textures.first().toString();
            }
        }
    }

    return model;
}

BongoModel BongoModelManager::loadSimpleFormat(const QString &dirPath)
{
    BongoModel model;
    model.path = dirPath;

    // 读取model.json配置
    QString configPath = dirPath + "/model.json";
    if (QFile::exists(configPath)) {
        QFile file(configPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            QJsonObject obj = doc.object();
            model.id = obj.value("id").toString();
            model.name = obj.value("name").toString();
        }
    }

    if (model.id.isEmpty()) {
        model.id = "user_" + QFileInfo(dirPath).fileName();
    }
    if (model.name.isEmpty()) {
        model.name = QFileInfo(dirPath).fileName();
    }

    // cover.png
    QString coverPath = dirPath + "/cover.png";
    if (QFile::exists(coverPath)) {
        model.coverImage = QPixmap(coverPath);
    }

    // background.png
    QString bgPath = dirPath + "/background.png";
    if (QFile::exists(bgPath)) {
        model.backgroundImage = QPixmap(bgPath);
    }

    // left_hand.png / right_hand.png
    QString leftHandPath = dirPath + "/left_hand.png";
    if (QFile::exists(leftHandPath)) {
        model.leftHandDownImage = QPixmap(leftHandPath);
    }

    QString rightHandPath = dirPath + "/right_hand.png";
    if (QFile::exists(rightHandPath)) {
        model.rightHandDownImage = QPixmap(rightHandPath);
    }

    // keys/ 目录
    QString keysDir = dirPath + "/keys";
    if (QDir(keysDir).exists()) {
        loadKeyImages(keysDir, model);
    }

    return model;
}

void BongoModelManager::loadKeyImages(const QString &keysDir, BongoModel &model)
{
    QDir dir(keysDir);
    if (!dir.exists()) return;

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    QStringList keyFiles = dir.entryList(filters, QDir::Files);

    for (const QString &file : keyFiles) {
        QString keyName = QFileInfo(file).baseName();
        QPixmap pix(keysDir + "/" + file);
        if (!pix.isNull()) {
            model.keyImages[keyName] = pix;
        }
    }
}

QList<BongoModel> BongoModelManager::models() const
{
    return m_models.values();
}

BongoModel BongoModelManager::currentModel() const
{
    return getModelById(m_currentModelId);
}

QString BongoModelManager::currentModelId() const
{
    return m_currentModelId;
}

bool BongoModelManager::setCurrentModel(const QString &id)
{
    if (!m_models.contains(id)) return false;
    m_currentModelId = id;
    saveCurrentModel(id);
    emit modelChanged(id);
    return true;
}

BongoModel BongoModelManager::getModelById(const QString &id) const
{
    auto it = m_models.find(id);
    if (it != m_models.end()) return it.value();
    return BongoModel();
}

bool BongoModelManager::importModel(const QString &sourcePath, const QString &modelName)
{
    QDir sourceDir(sourcePath);
    if (!sourceDir.exists()) return false;

    // 检查是否有cover.png（在根目录或resources目录下）
    bool hasCover = QFile::exists(sourcePath + "/cover.png") ||
                    QFile::exists(sourcePath + "/resources/cover.png");
    if (!hasCover) return false;

    // 生成唯一ID
    QString modelId = "user_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QString destPath = userModelsDirectory() + "/" + modelId;

    QDir destDir;
    if (!destDir.mkpath(destPath)) return false;

    // 复制整个目录
    QDir srcDir(sourcePath);
    QStringList allFiles = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &entry : allFiles) {
        QString srcEntry = sourcePath + "/" + entry;
        QString destEntry = destPath + "/" + entry;

        QFileInfo fi(srcEntry);
        if (fi.isDir()) {
            // 递归复制子目录
            QDir().mkpath(destEntry);
            QDir subSrc(srcEntry);
            QStringList subFiles = subSrc.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &subFile : subFiles) {
                QString srcFile = srcEntry + "/" + subFile;
                QString destFile = destEntry + "/" + subFile;
                if (QFileInfo(srcFile).isDir()) {
                    // 更深层的目录
                    QDir().mkpath(destFile);
                    QDir deepSrc(srcFile);
                    QStringList deepFiles = deepSrc.entryList(QDir::Files);
                    for (const QString &deepFile : deepFiles) {
                        QFile::copy(srcFile + "/" + deepFile, destFile + "/" + deepFile);
                    }
                } else {
                    QFile::copy(srcFile, destFile);
                }
            }
        } else {
            QFile::copy(srcEntry, destEntry);
        }
    }

    // 创建model.json
    QJsonObject config;
    config["id"] = modelId;
    config["name"] = modelName;
    config["importedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    config["sourceFormat"] = "auto";

    QFile configFile(destPath + "/model.json");
    if (configFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        configFile.write(doc.toJson());
        configFile.close();
    }

    // 重新加载
    loadModels();
    return true;
}

bool BongoModelManager::importBongoCatModel(const QString &sourcePath, const QString &modelName)
{
    return importModel(sourcePath, modelName);
}

bool BongoModelManager::deleteModel(const QString &id)
{
    auto it = m_models.find(id);
    if (it == m_models.end() || it.value().isPreset) return false;

    QDir dir(it.value().path);
    dir.removeRecursively();
    m_models.erase(it);

    if (m_currentModelId == id && !m_models.isEmpty()) {
        setCurrentModel(m_models.firstKey());
    }

    return true;
}

void BongoModelManager::saveCurrentModel(const QString &id)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + "/bongo_config.json";
    QJsonObject config;
    config["currentModel"] = id;

    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        file.write(doc.toJson());
        file.close();
    }
}

QString BongoModelManager::loadCurrentModelId()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + "/bongo_config.json";
    QFile file(configPath);
    if (!file.exists()) return "preset_bongocat_standard";

    if (!file.open(QIODevice::ReadOnly)) return "preset_bongocat_standard";

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    return doc.object().value("currentModel").toString("preset_bongocat_standard");
}
