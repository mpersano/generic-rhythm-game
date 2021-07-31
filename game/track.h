#pragma once

#include <memory>
#include <string>
#include <vector>

struct Track {
    struct Event {
        enum class Type {
            Tap,
            Hold,
        };
        Type type;
        int track;
        float start;
        float duration;
    };

    std::string audioFile;
    std::string title;
    std::string author;
    int beatsPerMinute;
    int eventTracks;
    std::vector<Event> events;
};

std::unique_ptr<Track> loadTrack(const std::string &jsonPath);
