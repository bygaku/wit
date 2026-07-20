#include "project_serializer.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QUuid>

namespace wit::studio {

namespace {
constexpr const char* WSP_FORMAT_VERSION = "1.0";

/* =====================================================================
 * UUID helpers
 * ===================================================================== */

QString UuidToString(const binary::Uuid& uuid) {
    return QString::fromStdString(uuid.ToHexString());
}

bool UuidFromString(const QString& text, binary::Uuid& out) {
    const QUuid parsed = QUuid::fromString(text);
    if (parsed.isNull()) return false;
    const QByteArray bytes = parsed.toRfc4122();
    out = binary::Uuid::FromBytes(reinterpret_cast<const uint8_t*>(bytes.constData()));
    return true;
}

binary::Uuid GenerateUuid() {
    const QByteArray bytes = QUuid::createUuid().toRfc4122();
    return binary::Uuid::FromBytes(reinterpret_cast<const uint8_t*>(bytes.constData()));
}

/* =====================================================================
 * Enum <-> string mappings (the .wsp is human-readable on purpose)
 * ===================================================================== */

QString CueTypeToString(CueType type) {
    switch (type) {
        case CueType::SHUFFLE:    return "shuffle";
        case CueType::SEQUENTIAL: return "sequential";
        case CueType::POLYPHONIC:
        default:                  return "polyphonic";
    }
}

bool CueTypeFromString(const QString& text, CueType& out) {
    if (text == "polyphonic") { out = CueType::POLYPHONIC; return true; }
    if (text == "shuffle")    { out = CueType::SHUFFLE;    return true; }
    if (text == "sequential") { out = CueType::SEQUENTIAL; return true; }
    return false;
}

QString StreamingModeToString(StreamingMode mode) {
    return (mode == StreamingMode::STREAMING) ? "streaming" : "memory";
}

bool StreamingModeFromString(const QString& text, StreamingMode& out) {
    if (text == "memory")    { out = StreamingMode::MEMORY_RESIDENT; return true; }
    if (text == "streaming") { out = StreamingMode::STREAMING;       return true; }
    return false;
}

/* =====================================================================
 * Save Models (Serialize)
 * ===================================================================== */

QJsonObject RandomizeToJson(const RandomizeRange& range) {
    QJsonObject json;
    json["enabled"] = range.enabled;
    json["min"]     = range.min;
    json["max"]     = range.max;
    return json;
}

QJsonObject CueToJson(const CueModel& cue) {
    QJsonObject json;
    json["cue_id"]         = static_cast<qint64>(cue.cue_id);
    json["cue_name"]       = QString::fromStdString(cue.cue_name);
    json["category_id"]    = cue.category_id;
    json["cue_type"]		  = CueTypeToString(cue.cue_type);
    json["loop_enabled"]   = cue.loop_enabled;
    json["streaming_mode"] = StreamingModeToString(cue.streaming_mode);
    json["base_volume"]    = cue.base_volume;
    json["base_pitch"]     = cue.base_pitch;
    json["volume_random"]  = RandomizeToJson(cue.volume_random);
    json["pitch_random"]   = RandomizeToJson(cue.pitch_random);

    QJsonArray waveforms;
    for (const auto& wf : cue.waveforms) {
        QJsonObject wf_json;
        wf_json["file_path"]		= QString::fromStdString(wf.file_path);
        wf_json["display_name"]  = QString::fromStdString(wf.display_name);
        waveforms.append(wf_json);
    }
    json["waveforms"] = waveforms;
    return json;
}

QJsonDocument ProjectToJson(const ProjectModel& project) {
    QJsonObject root;
    root["format_version"] = WSP_FORMAT_VERSION;
    root["project_uuid"]   = UuidToString(project.project_uuid);
    root["project_name"]   = QString::fromStdString(project.project_name);
    root["next_cue_id"]    = static_cast<qint64>(project.next_cue_id);

    QJsonObject format;
    format["sample_rate"]   = static_cast<qint64>(project.audio_format.sample_rate);
    format["bit_depth"]     = project.audio_format.bit_depth;
    format["sample_format"] = project.audio_format.sample_format == SampleFormat::FLOAT ? "float" : "int";
    format["channels"]      = project.audio_format.channels;
    root["audio_format"]	   = format;

    QJsonObject ducking;
    ducking["fade_in_ms"]  = static_cast<qint64>(project.ducking.fade_in_ms);
    ducking["fade_out_ms"] = static_cast<qint64>(project.ducking.fade_out_ms);
    root["ducking"] = ducking;

    QJsonArray categories;
    for (const auto& cate : project.categories) {
        QJsonObject cate_json;
        cate_json["id"]                     = cate.id;
        cate_json["name"]                   = QString::fromStdString(cate.name);
        cate_json["is_preset"]              = cate.is_preset;
        cate_json["static_volume_db"]       = cate.static_volume_db;
        cate_json["ducking_attenuation_db"] = cate.ducking_attenuation_db;
        cate_json["is_ducker"]              = cate.is_ducker;
        categories.append(cate_json);
    }
    root["categories"] = categories;

    QJsonArray collections;
    for (const auto& collection : project.cue_collections) {
        QJsonObject col_json;
        col_json["name"]      = QString::fromStdString(collection.name);
        col_json["wccb_uuid"] = UuidToString(collection.wccb_uuid);

        QJsonArray cues;
        for (const auto& cue : collection.cues) {
            cues.append(CueToJson(cue));
        }
        col_json["cues"] = cues;
        collections.append(col_json);
    }
    root["cue_collections"] = collections;

    return QJsonDocument(root);
}

/* =====================================================================
 * Load Models (Deserialize)
 * ===================================================================== */

bool RandomizeFromJson(const QJsonObject& json, RandomizeRange& out) {
    out.enabled = json["enabled"].toBool(false);
    out.min     = static_cast<float>(json["min"].toDouble(1.0));
    out.max     = static_cast<float>(json["max"].toDouble(1.0));
    return true;
}

bool CueFromJson(const QJsonObject& json, CueModel& out, std::string& out_error) {
    out.cue_id      = static_cast<uint32_t>(json["cue_id"].toInteger(0));
    out.cue_name    = json["cue_name"].toString().toStdString();
    out.category_id = static_cast<uint16_t>(json["category_id"].toInt(0));

    if (!CueTypeFromString(json["cue_type"].toString("polyphonic"), out.cue_type)) {
        out_error = "未定義の CueType: '" + out.cue_name + "' が登録されています";
        return false;
    }

    out.loop_enabled = json["loop_enabled"].toBool(true);
    if (!StreamingModeFromString(json["streaming_mode"].toString("memory"),
                                 out.streaming_mode)) {
        out_error = "未定義の StreamingMode: '" + out.cue_name + "' が登録されています";
        return false;
    }

    out.base_volume = static_cast<float>(json["base_volume"].toDouble(1.0));
    out.base_pitch  = static_cast<float>(json["base_pitch"].toDouble(1.0));
    RandomizeFromJson(json["volume_random"].toObject(), out.volume_random);
    RandomizeFromJson(json["pitch_random"].toObject(), out.pitch_random);

    for (const auto& wf_value : json["waveforms"].toArray()) {
        const QJsonObject wf_json = wf_value.toObject();
        WaveformModel wf;
        wf.file_path	 = wf_json["file_path"].toString().toStdString();
        wf.display_name  = wf_json["display_name"].toString().toStdString();
        out.waveforms.push_back(std::move(wf));
    }

    return true;
}

bool ProjectFromJson(const QJsonDocument& doc, ProjectModel& out, std::string& out_error) {
    if (!doc.isObject()) {
        out_error = "Jsonオブジェクトではありません";
        return false;
    }

	const QJsonObject root = doc.object();

    if (!UuidFromString(root["project_uuid"].toString(), out.project_uuid)) {
        out_error = "project_uuid が存在しない、または無効です";
        return false;
    }
    out.project_name = root["project_name"].toString().toStdString();

    const QJsonObject format = root["audio_format"].toObject();
    out.audio_format.sample_rate = static_cast<uint32_t>(format["sample_rate"].toInteger(48000));
    out.audio_format.bit_depth   = static_cast<uint16_t>(format["bit_depth"].toInt(16));
    out.audio_format.sample_format = format["sample_format"].toString("int") == "float" ? SampleFormat::FLOAT : SampleFormat::INT;
    out.audio_format.channels = static_cast<uint8_t>(format["channels"].toInt(2));

    const QJsonObject ducking = root["ducking"].toObject();
    out.ducking.fade_in_ms  = static_cast<uint32_t>(ducking["fade_in_ms"].toInteger(200));
    out.ducking.fade_out_ms = static_cast<uint32_t>(ducking["fade_out_ms"].toInteger(500));

    for (const auto& cate_value : root["categories"].toArray()) {
        const QJsonObject cate_json = cate_value.toObject();
        CategoryInfo cate;
        cate.id                     = static_cast<uint16_t>(cate_json["id"].toInt(0));
        cate.name                   = cate_json["name"].toString().toStdString();
        cate.is_preset              = cate_json["is_preset"].toBool(false);
        cate.static_volume_db       = static_cast<float>(cate_json["static_volume_db"].toDouble(0.0));
        cate.ducking_attenuation_db = static_cast<float>(cate_json["ducking_attenuation_db"].toDouble(-6.0));
        cate.is_ducker              = cate_json["is_ducker"].toBool(false);
        out.categories.push_back(std::move(cate));
    }

    uint32_t max_cue_id = 0;
    for (const auto& col_value : root["cue_collections"].toArray()) {
        const QJsonObject col_json = col_value.toObject();
    	CueCollectionModel collection;

        collection.name = col_json["name"].toString().toStdString();
        if (!UuidFromString(col_json["wccb_uuid"].toString(), collection.wccb_uuid)) {
            out_error = "wccb_uuid が存在しない、または無効です: '" + collection.name + "'";
            return false;
        }

        for (const auto& cue_value : col_json["cues"].toArray()) {
            CueModel cue;
            if (!CueFromJson(cue_value.toObject(), cue, out_error)) return false;
            max_cue_id = std::max(max_cue_id, cue.cue_id);
            collection.cues.push_back(std::move(cue));
        }

    	out.cue_collections.push_back(std::move(collection));
    }

    const uint32_t stored_next = static_cast<uint32_t>(root["next_cue_id"].toInteger(0));
    out.next_cue_id = std::max(stored_next, max_cue_id + 1);

    return true;
}

}  // namespace

bool ProjectSerializer::Load(const std::filesystem::path& path, ProjectModel& out, std::string& out_error) {
    out = ProjectModel{};
    out_error.clear();

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly)) {
        out_error = "ファイルを開くことができませんでした: " + path.string();
        return false;
    }

    QJsonParseError parse_error{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        out_error = "JSONの解析エラー: " + parse_error.errorString().toStdString();
        return false;
    }

    return ProjectFromJson(doc, out, out_error);
}

bool ProjectSerializer::SaveToFile(const std::filesystem::path& path, const ProjectModel& project, std::string& out_error) {
    out_error.clear();

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        out_error = "書き込み用にファイルを開くことができませんでした: " + path.string();
        return false;
    }

    const QByteArray bytes = ProjectToJson(project).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        out_error = "書き込みエラーが発生しました: " + path.string();
        return false;
    }

    return true;
}

ProjectModel ProjectSerializer::MakeNewProject(const std::string& project_name) {
    ProjectModel project;

    project.project_uuid = GenerateUuid();
    project.project_name = project_name;
    project.audio_format = { 48000, 16, SampleFormat::INT, 2 };
    project.categories   = {
        { "Serif",       0,  0.0f, -6.0f, true, true  },
        { "UI",          1,  0.0f, -6.0f, true, false },
        { "Other SE",    2,  0.0f, -6.0f, true, false },
        { "BGM",         3, -3.0f, -6.0f, true, false },
        { "Environment", 4, -6.0f, -6.0f, true, false },
    };

    return project;
}

CueCollectionModel ProjectSerializer::MakeNewCollection(const std::string& name) {
    CueCollectionModel collection;

    collection.name      = name;
    collection.wccb_uuid = GenerateUuid();

	return collection;
}

}
