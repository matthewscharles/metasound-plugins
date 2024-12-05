| Date   | Activities                                                          |
|--------|---------------------------------------------------------------------|
| 10/16  | - Initial commits.<br>- Adapt tutorial code<br>- Work through issues with building. |
| 10/17  | - First working version of sample hold node<br>- Tested in UE 5.4 (Windows). |
| 10/18  | - Rework code and documentation.<br>- Add second node to plugin (sample hold bank)<br>- Test in branch. |
| 10/19  | - Merge sample hold bank to main.<br>- Start building shift register node/refactoring sample hold bank (on hold)<br>- Continue gathering documentation<br>- Add dust node. |
| 10/20  | - Refine documentation and folder structure<br>- Add Slew node. | 
| 10/21  | - Start on node with trigger output. |
| 10/22  | - Refine documentation structure, start wiki. <br>- Investigate existing nodes and practices. | 
| 11/20  | - Work on doucmentation, wishlist. | 
| 11/21  | - Test Mac build process and nodes (UE 5.5). |
| 11/22  | - Update documentation<br>- Test alternating polarity on Dust node, merge with main.<br>- Add width node. |
| 11/23  | - Add balance node<br>- Match existing conventions. |
| 11/24  | - Merge width and balance with main. |
| 11/25  | - Create first draft of a Wave Reader node (sample-by-sample from an audio input). |
| 11/26  | - Add stereo crossfade node. |
| 11/27  | - Convert stereo crossfade to equal power, merge with main.<br>- Remove balance node (already available in spatialisation->stereo pan). Code: [e48d011](https://github.com/matthewscharles/metasound-plugins/commit/e48d011755f95b5e6f1e72648e1d3b9dfbc4392d) : [MetasoundBalanceNode.cpp](https://github.com/matthewscharles/metasound-plugins/blob/e48d011755f95b5e6f1e72648e1d3b9dfbc4392d/Source/MetasoundBranches/Private/MetasoundBalanceNode.cpp).<br>- Add bool To audio node.<br>- Create first draft of EDO node.|
| 11/28  | - Create first draft of tuning node.  Note: I think these two latest nodes will run on the audio thread unless we include a trigger control.  |
| 11/29  | - Test EDO, switch input from float to integer, merge into main.  |
| 11/30  | - Create click node, test, merge into main.  |
| 12/01  | - Merge tuning into main.<br>- Update documentation, shift order of readme.  |
| 12/02  | - Test build process and all nodes on UE5.5 Windows and Mac.<br>- Clean up comments and update documentation.  |
| 12/03  | - Add clock divider node and Metasound patch example.<br>- Finish phase disperser and merge into main.  |
| 12/04  | - Attempt to update code to new API (replace old branch, not much luck yet).<br>-Add categories to readme.<br>-Remove Sample and Hold bank from collection.  |