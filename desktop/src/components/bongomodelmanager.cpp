#include "bongomodelmanager.h"
#include <QDateTime>
#include <QRandomGenerator>

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

    // 加载预设模型(从Qt资源)
    BongoModel preset = loadPresetFromResources();
    if (preset.isValid()) {
        m_models[preset.id] = preset;
    }

    // 加载用户自定义模型
    loadUserModels();

    // 如果当前模型无效，使用第一个
    if (!m_models.contains(m_currentModelId) && !m_models.isEmpty()) {
        m_currentModelId = m_models.firstKey();
    }

    emit modelsLoaded();
}

BongoModel BongoModelManager::loadPresetFromResources()
{
    BongoModel model;
    model.id = "preset_default_cat";
    model.name = QStringLiteral("经典猫咪 (默认)");
    model.isPreset = true;

    // 从Qt资源加载
    model.coverImage = QPixmap(":/assets/bongo/cover.png");
    model.backgroundImage = QPixmap(":/assets/bongo/background.png");

    // 加载按键图片
    static const QMap<QString, QString> keyMap = {
        {"KeyA", "KeyA"}, {"KeyB", "KeyB"}, {"KeyC", "KeyC"}, {"KeyD", "KeyD"},
        {"KeyE", "KeyE"}, {"KeyF", "KeyF"}, {"KeyG", "KeyG"}, {"KeyQ", "KeyQ"},
        {"KeyR", "KeyR"}, {"KeyS", "KeyS"}, {"KeyT", "KeyT"}, {"KeyV", "KeyV"},
        {"KeyW", "KeyW"}, {"KeyX", "KeyX"}, {"KeyZ", "KeyZ"},
        {"Num1", "Num1"}, {"Num2", "Num2"}, {"Num3", "Num3"},
        {"Num4", "Num4"}, {"Num5", "Num5"},
        {"Space", "Space"}, {"Return", "Return"}, {"Shift", "Shift"},
        {"Control", "Control"}, {"Alt", "Alt"}
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
    if (!file.exists()) return "preset_default_cat";

    if (!file.open(QIODevice::ReadOnly)) return "preset_default_cat";

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    return doc.object().value("currentModel").toString("preset_default_cat");
}
