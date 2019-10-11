#ifndef _MUSIC_H
#define _MUSIC_H

typedef struct
{
	int pitch; 
	int dimation;
} note;

// 1		2		3		4		5		6       7
// C		D		E		F		G		A	B
// 261.6256	293.6648	329.6276	349.2282	391.9954	440	493.8833

// C调
#define DO	262
#define RE	294
#define MI	330
#define FA	349
#define SOL	392
#define LA	440
#define SI	494

#define BEAT	(60000000 / 120)

const note HappyNewYear[] = {
	{DO,   BEAT/2}, {DO,   BEAT/2}, {DO,   BEAT}, {SOL/2, BEAT},
	{MI,   BEAT/2}, {MI,   BEAT/2}, {MI,   BEAT}, {DO,    BEAT},
	{DO,   BEAT/2}, {MI,   BEAT/2}, {SOL,  BEAT}, {SOL,    BEAT},
	{FA,   BEAT/2}, {MI,   BEAT/2}, {RE,   BEAT}, {RE,    BEAT},
	{RE,   BEAT/2}, {MI,   BEAT/2}, {FA,   BEAT}, {FA,    BEAT},
	{MI,   BEAT/2}, {RE,   BEAT/2}, {MI,   BEAT}, {DO,    BEAT},
	{DO,   BEAT/2}, {MI,   BEAT/2}, {RE,   BEAT}, {SOL/2, BEAT},
	{SI/2, BEAT/2}, {RE,   BEAT/2}, {DO,   BEAT}, {DO,    BEAT},
};

#endif
