#include "track.h"

#include <gx/ioutil.h>

#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

std::unique_ptr<Track> loadTrack(const std::string &jsonPath)
{
    const auto json = GX::Util::readFile(jsonPath);
    if (!json) {
        spdlog::warn("Could not read track file {}", jsonPath);
        return {};
    }

    rapidjson::Document document;
    document.Parse(reinterpret_cast<const char *>(json->data()));
    if (document.HasParseError()) {
        spdlog::warn("Failed to parse track file {}", jsonPath);
        return {};
    }

    auto track = std::make_unique<Track>();

    track->audioFile = document["audioFile"].GetString();
    track->beatsPerMinute = document["beatsPerMinute"].GetFloat();
    track->eventTracks = document["eventTracks"].GetInt();
    track->title = document["title"].GetString();
    track->author = document["author"].GetString();

    const auto &eventsArray = document["events"];
    assert(eventsArray.IsArray());

    auto &events = track->events;
    events.reserve(eventsArray.Size());
    for (int i = 0, size = eventsArray.Size(); i < size; ++i) {
        const auto &eventSettings = eventsArray[i];
        Track::Event event;
        event.type = static_cast<Track::Event::Type>(eventSettings["type"].GetInt());
        event.track = eventSettings["track"].GetInt();
        event.start = eventSettings["start"].GetFloat();
        event.duration = eventSettings["duration"].GetFloat();
        events.push_back(event);
    }

    return track;
}
