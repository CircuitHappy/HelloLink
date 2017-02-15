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
  enum InPin {
      PlayStop = 1
  };

  enum OutPin {
      Clock = 25,
      Reset = 28,
      PlayIndicator = 4
  };

  enum PlayState {
      Stopped,
      Cued,
      Playing
  };

  struct State {
      ableton::Link link;
      std::atomic<bool> running;
      std::atomic<bool> playPressed;
      std::atomic<PlayState> playState;

      State()
        : link(120.0)
        , running(true)
        , playPressed(false)
        , playState(Stopped)
      {
        link.enable(true);
      }
  };

  void configurePins() {
      wiringPiSetup();
      pinMode(PlayStop, INPUT);
      pullUpDnControl(PlayStop, PUD_DOWN);
      pinMode(Clock, OUTPUT);
      pinMode(Reset, OUTPUT);
      pinMode(PlayIndicator, OUTPUT);
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

  void outputClock(double beats, double phase, double tempo) {
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
  }

  void input(State& state) {
      while (state.running) {

          const bool playPressed = digitalRead(PlayStop) == HIGH;
          if (playPressed && !state.playPressed) {
              switch (state.playState) {
                  case Stopped:
                      state.playState.store(Cued);
                      break;
                  case Cued:
                  case Playing:
                      state.playState.store(Stopped);
                      break;
              }
          }

          state.playPressed.store(playPressed);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
  }

  void output(State& state) {
      while (state.running) {
          const auto time = state.link.clock().micros();
          auto timeline = state.link.captureAppTimeline();

          const double beats = timeline.beatAtTime(time, QUANTUM);
          const double phase = timeline.phaseAtTime(time, QUANTUM);
          const double tempo = timeline.tempo();

          switch (state.playState) {
              case Cued: {
                      // Tweak this
                      const bool playHigh = (long)(beats * 2) % 2 == 0;
                      digitalWrite(PlayIndicator, playHigh ? HIGH : LOW);
                      if (phase <= 0.01) {
                          state.playState.store(Playing);
                      }
                  break;
              }
              case Playing:
                  digitalWrite(PlayIndicator, HIGH);
                  outputClock(beats, phase, tempo);
                  break;
              default:
                  digitalWrite(PlayIndicator, LOW);
                  break;
          }

          std::this_thread::sleep_for(std::chrono::microseconds(250));
      }
  }
}

int main(void) {
    configurePins();
    State state;

    std::thread inputThread(input, std::ref(state));
    std::thread outputThread(output, std::ref(state));

    while (state.running) {
        const auto time = state.link.clock().micros();
        auto timeline = state.link.captureAppTimeline();
        printState(time, timeline);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    inputThread.join();
    outputThread.join();

    return 0;
}

