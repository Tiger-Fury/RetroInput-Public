# Multiple Xbox/XInput controllers

The dedicated Xbox/XInput reader polls all four slots, including when slot 0 is
empty. In Controller Assist mode, connected pads share control.

| Input | Combination |
|---|---|
| Buttons, D-pad, stick clicks | Currently held buttons combine |
| Triggers | Strongest value for each trigger |
| Both sticks | One pad owns both, based on its latest meaningful movement |

A 2048-unit movement threshold filters stick jitter. Buttons and triggers do not
change stick ownership. If the owner centres its sticks or disconnects, another
deflected pad can take over. Disconnected pads stop contributing immediately.

Between Xbox input and native controllers, the most recently active device is
selected. Releasing controls keeps that selection within the 30-second activity
window; keyboard input takes priority. Native readers are selected separately,
not merged into the Xbox group. Player assignment and multiplayer support depend
on the game.
