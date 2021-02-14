Name: Hyun Bum Cho
I implemented all of the required functionality.
For the simplification, I opted for the simple edge midpoint as optimal. For collapse_edge, I skip edges that don't have a good resolution when collapsing - particularly, when there are edges (aside from the ones residing on the same faces) that would end up having the same vertices. I think there was a discussion of this on piazza.
My model.dae is supposed to be a spikeball of some sort.