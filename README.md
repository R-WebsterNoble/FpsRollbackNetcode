# FPS ROLLBACK NETCODE
I have been toying with an idea for a while and I have made a basic proof of concept I can
demonstrate. Though I'm in no means suggesting it as an off-the-shelf solution to make your net
code better if it is plugged straight in as there are some pretty wide sweeping changes that would
need to be implemented, especially to any existing products. Hopefully, it holds some promise
however, though I cannot vouch for its performance and scalability at this stage.
The idea came to me after first hand experiences with desynchronizations within games between
the client and the server. Example : https://youtu.be/tMBJ1QN0tXI?t=13. I theorised that aspects of
the peer-to-peer and trustless system used in cryptocurrency, could be used to run a multiplayer
game. Instead of transactions put into blocks instead it's player inputs (such as 'look_x: -10' and
'+move_forward') put into game ticks. It turns out I am not the first to have come up with this idea,
(called Rollback Netcode) this is already in use in some online arcade style fighting games. Although I
am not aware of anyone having attempted to implement this in an FPS game or any game in which
there are more than 2 players.
My primary objective for experimenting with this this isn't to create a direct improvement over
existing FPS netcode implementations for the basic PvP scenario. (I think existing implications of that
are going to be hard to beat.) But I believe this solution has the potential to significantly improve
how a game feels and make the experience more consistent for people with high latency or unstable
internet. Especially for games with AI or physics. I also believe this approach has the potential to
better handle the edge cases caused by network inconsistency (such as seen in my example) and it
theoretically should be more reliable at producing a consistent online gameplay experience for
players.
## TECHNICAL DETAILS
For every game tick I take a previous world state and a set of player inputs and deterministically
calculate a resulting next world state. For networked clients however I will probably not have
received all the latest inputs due to network delays. Therefore I input predicted inputs instead. A set
number of previous world states, are stored in a buffer and when inputs are received from a player
for a tick in the past the game state is "rolled back" for that tick and re-calculated with the non-
predicted inputs. The subsequent game ticks are then re-calculated with more up to date predictions
and the result is drawn. One issue here is when this occurs, the resulting current position of a player
may suddenly appear to teleport. To address this I created a "smoothed" player representation that
more smoothly moves when these large discrepancies take place between the predicted and
updated player positions. This smoothing can be fine-tuned with a delay which shows a set number
of game ticks in the past. This means that while the current display may not be the most current
state of the world there are less predictions present which can be adjusted in real-time making the
overall game feel consistent.
## ADVANTAGES
- Feels like a single player game - For everything that isn't directly affected by other players
the game will behave like it's running locally. So even with very poor internet, the player's
own movement, AI, and physics will behave as if they are playing a single-player game.
- No synchronous game server required - This approach does not have a need for a game
server to actually run the game in real-time. While direct peer-to-peer connections might
work a server is probably needed to facilitate connecting clients who are behind firewalls.
Also it would probably make sense to run the game to validate the outcome. Although this
does not need to be real-time.
- Trustless -- The only thing a client can do is send valid game inputs. Therefore there is no
opportunity for a hacked client to do something that a legitimate client can not do (For
example: Teleport, Shoot through solid objects, or move faster than intended).
- Tiny network packet sizes - The network packets in both directions only contain input data.
This data is tiny and this allows me to send duplicate data (potentially through different
network paths) with little concern for bandwidth limitations.
- Build in replays and easy debuggability -- If the entire game logic is deterministic then
recreating a bug would just be a case of just running a replay of the bug with a debugger
attached.
## PERFORMANCE CHALLENGES
Sounds pretty good right? Soo what's the catch?
- Client performance requirements -- For this to work the game must be able to re calculate a
number of game ticks within the time of 1 game ticks. For example, if an input for a game
tick arrives 10 ticks in the past, the game client must rollback those 10 ticks, re-calculate
them, and then display the next tick. The problems arises when the recalculation process
takes longer than one tick's worth of time. For instance, if one tick corresponds to 10ms of
game time, and the recalculation of 10 ticks requires 15ms of processing time, the
recalculation won't be completed and the game won't be able to draw the most up to date
game state.
- Increased client RAM usage -- The game client must store enough entire copies of the game
state in a buffer to handle rollbacks going back a certain number of ticks in the past.
I think that these technical challenges can be mitigated with some careful game design. Such as
much of the data needed to calculate game state could be made read-only and therefore shared
across all game states.
Anything that only has a visual effect (such as particles) can be moved out of the game state and
processed separately. It may also be possible to use differential game states to reduce RAM usage.
## OTHER CHALLENGES
- Audio / animations -- What if a sound effect is started on a predicted tick but after a resync
whatever caused that sound effect to start didn't actually happen?
- Hit validation on smoothed players -- The smoothed players only exist locally on each game
client but we want players to be able to shoot them. How do we allow this without blindly
trusting the client?
## CONCLUSION
My proof of concept is in early stages but does successfully demonstrates the concept. It also shows
that this it's capable of matching the performance of the CryEngine netcode implementation with
some potential advantages. Further work needs to be done to test the performance and scalability\
of the system especially when more players, physics and AI is involved.
