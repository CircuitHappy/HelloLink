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

  const double PULSES_PER_BEAT = 1.0;
  const double RESET_WIDTH = 1.0 / 16.0; // pct of beat
  const double QUANTUM = 4;

  enum OutPin {
      Clock = 22,
      Reset = 23
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
          << " | " << std::fixed << "beats: " << beats;
      clearLine();
  }

  void output(State& state) {
      while (state.running) {
          const auto time = state.link.clock().micros();
          auto timeline = state.link.captureAppTimeline();

          const auto beats = timeline.beatAtTime(time, QUANTUM);
          const auto phase = timeline.phaseAtTime(time, QUANTUM);

          double intgarbage;
          const auto beatFraction = std::modf(beats * PULSES_PER_BEAT, &intgarbage);
          const bool gateHigh = (beatFraction <= 0.5);

          digitalWrite(Clock, gateHigh ? HIGH : LOW);

          // Experiment with PW here
          const bool resetHigh = (phase <= RESET_WIDTH);

          digitalWrite(Reset, resetHigh ? HIGH : LOW);

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

