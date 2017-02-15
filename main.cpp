#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <ableton/Link.hpp>

extern "C" {
    #include <wiringPi.h>
}

using namespace std;

namespace {

  const double PULSES_PER_BEAT = 4.0;
  const double PULSE_LENGTH = 0.015; // seconds
  const double QUANTUM = 4;

  // Using WiringPi numbering scheme
  enum OutPin {
      Clock = 25,
      Reset = 28
  };

  struct State {
      std::atomic<bool> running;
      ableton::Link link;

      State()
        : running(true)
        , link(120.0)
      {
        link.enable(true);
      }
  };

  void configurePins() {
      wiringPiSetup();
      pinMode(Clock, OUTPUT);
      pinMode(Reset, OUTPUT);
  }

  void clearLine() {
      std::cout << "   \r" << std::flush;
      std::cout.fill(' ');
  }

  void printState(const std::chrono::microseconds time,
      const ableton::Link::Timeline timeline)
  {
      const auto beats = timeline.beatAtTime(time, QUANTUM);
      const auto phase = timeline.phaseAtTime(time, QUANTUM);
      std::cout << "tempo: " << timeline.tempo()
          << " | " << std::fixed << "beats: " << beats
          << " | " << std::fixed << "phase: " << phase;
      clearLine();
  }

  void output(State& state) {
      while (state.running) {
          const auto time = state.link.clock().micros();
          auto timeline = state.link.captureAppTimeline();

          const double beats = timeline.beatAtTime(time, QUANTUM);
          const double phase = timeline.phaseAtTime(time, QUANTUM);
          const double tempo = timeline.tempo();
          const double secondsPerBeat = 60.0 / tempo;

          // Fractional portion of current beat value
          double intgarbage;
          const auto beatFraction = std::modf(beats * PULSES_PER_BEAT, &intgarbage);

          // Fractional beat value for which clock should be high
          const auto highFraction = PULSE_LENGTH / secondsPerBeat;

          const bool resetHigh = (phase <= highFraction);
          digitalWrite(Reset, resetHigh ? HIGH : LOW);

          const bool clockHigh = (beatFraction <= highFraction);
          digitalWrite(Clock, clockHigh ? HIGH : LOW);

          std::this_thread::sleep_for(std::chrono::microseconds(250));
      }
  }
}

int main(void) {
    configurePins();
    State state;

    std::thread outputThread(output, std::ref(state));

    while (state.running) {
        const auto time = state.link.clock().micros();
        auto timeline = state.link.captureAppTimeline();
        printState(time, timeline);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}

