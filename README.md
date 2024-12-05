# MetaSound-Plugins 
This repository contains a collection of custom nodes for Unreal Engine's MetaSounds, which I am working toward releasing as a plugin (working title: "Branches").

## Branches
There are currently twelve nodes available in the *MetaSoundBranches* plugin for testing, with several more in development:

| Node                                | Description                                                                                                         | Category              |
|-------------------------------------|---------------------------------------------------------------------------------------------------------------------|-----------------------|
| `Bool To Audio`                     | Convert a boolean value to an audio signal, with optional rise and fall times.                                      | Conversions     |
| `Clock Divider`                     | Divide a trigger into eight density levels.                                                                         | Triggers      |
| `Crossfade (stereo)`                | A crossfader for stereo signals.                                                                                    | Mix        |
| `Click`                             | Trigger a one-sample impulse (unipolar or alternating polarity per impulse).                                                  | Generators      |
| `Dust`                              | A randomly timed impulse generator (unipolar or alternating polarity per impulse) with density control and audio-rate modulation. | Generators           |
| `EDO`                               | Generate frequencies for tuning systems using equally divided octaves (float) with a MIDI note input. Set a reference frequency and reference MIDI note (defaults to A440). | Tuning  |
| `Phase Disperser`                   | A chain of allpass filters to soften transients and add that classic laser/slinky-style effect.                      | Filters               |
| `Sample And Hold (audio trigger)`   | An alternative sample and hold unit with an audio-rate trigger.                                                     | Filters |
| `Shift Register`                    | An eight-stage shift register for floats.                                                                           | Modulation  |
| `Slew`                              | A slew limiter to smooth out the rise and fall times of an audio signal.                                             | Envelopes     |
| `Tuning`                            | Quantize a float value to a custom 12-note tuning, with adjustment per-note.                                         | Tuning   |
| `Width`                             | Stereo width adjustment (0-200%), using mid-side processing.                                                        | Spatialization        |

Upon installing the plugin, these items will appear in the sub-category `Branches` within the Metasound `Functions` category.

![Screenshot of a selection of custom nodes in Metasound, as listed in the table above](./docs/nodes.png)
![Signal flow in a MetaSound Source showing a sample and hold node connected to two audio sources, controlling the volume of a white noise generator.](./docs/SaH_demo.png)

## Installation
- Clone the repository into your chosen Unreal Engine project's `Plugins` directory.
- Build the project using the [Unreal Automation Tool](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-automation-tool-for-unreal-engine):

### Windows
- In PowerShell, navigate to the `Engine\Build\BatchFiles\` folder, and run: 
```PowerShell
.\RunUAT.bat BuildPlugin -plugin="<path to your project>\Plugins\metasound-plugins\MetasoundBranches.uplugin" -package="<path to your project>\Plugins\metasound-plugins\MetasoundBranches.uplugin"
```
### Mac
- In terminal, navigate to the `Engine/Build/BatchFiles/` folder, and run: 
```Bash
sudo ./RunUAT.sh BuildPlugin -plugin="<path to your project>/Plugins/metasound-plugins/MetasoundBranches.uplugin" -package="<path to your project>/Plugins/metasound-plugins/MetasoundBranches.uplugin"
```

Please see [Learning / Building Plugins](https://dev.epicgames.com/community/learning/tutorials/qz93/unreal-engine-building-plugins) for more information.

## Documentation
Development notes:
- [Initial notes](./docs/README.md): information on the first custom node and development process.
- [Quick reference](./docs/quick_reference.md): some development notes for creating and troubleshooting custom nodes. 

---

## Links
- [MetaSounds Reference Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/metasounds-reference-guide-in-unreal-engine)
- [MetaSounds Tutorial videos](https://dev.epicgames.com/community/learning/recommended-community-tutorial/Kw7l/unreal-engine-metasounds) (Matt Spendlove)
- [Creating MetaSound Nodes in C++ Quickstart](https://dev.epicgames.com/community/learning/tutorials/ry7p/unreal-engine-creating-metasound-nodes-in-c-quickstart) (Anna Lantz)