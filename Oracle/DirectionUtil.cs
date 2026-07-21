using System;
using UnityEngine;

// Token: 0x0200009A RID: 154
public static class DirectionUtil
{
	// Token: 0x0600094D RID: 2381 RVA: 0x000201DA File Offset: 0x0001E5DA
	public static bool LeftOf(this Direction a, Direction b)
	{
		if (a.Ortho() && b.Ortho())
		{
			return a.RotClockwise90() == b;
		}
		return DirectionUtil.ContinueRot(a, b) == a.RotClockwise90();
	}

	// Token: 0x0600094E RID: 2382 RVA: 0x0002020B File Offset: 0x0001E60B
	public static Direction Cross(this Direction a, Direction b)
	{
		return (Direction)DirectionUtil.crosses[(int)a, (int)b];
	}

	// Token: 0x0600094F RID: 2383 RVA: 0x00020219 File Offset: 0x0001E619
	public static bool Vertical(this Direction a)
	{
		return a > Direction.None;
	}

	// Token: 0x06000950 RID: 2384 RVA: 0x0002021F File Offset: 0x0001E61F
	public static bool Horizontal(this Direction a)
	{
		return a <= Direction.None;
	}

	// Token: 0x06000951 RID: 2385 RVA: 0x00020228 File Offset: 0x0001E628
	public static bool Flat(this Direction a)
	{
		return a < Direction.None;
	}

	// Token: 0x06000952 RID: 2386 RVA: 0x0002022E File Offset: 0x0001E62E
	public static Quaternion ToQuat(this Direction d)
	{
		return DirectionUtil.dirrots[(int)d];
	}

	// Token: 0x06000953 RID: 2387 RVA: 0x00020240 File Offset: 0x0001E640
	public static Direction RotBetween(Direction a, Direction b)
	{
		return (Direction)DirectionUtil.rotbetweens[(int)a, (int)b];
	}

	// Token: 0x06000954 RID: 2388 RVA: 0x0002024E File Offset: 0x0001E64E
	public static bool ParallelTo(this Direction a, Direction b)
	{
		return a.Valid() && b.Valid() && (a == b || a.Inverse() == b);
	}

	// Token: 0x06000955 RID: 2389 RVA: 0x0002027B File Offset: 0x0001E67B
	public static bool OrthoTo(this Direction a, Direction b)
	{
		return !a.ParallelTo(b);
	}

	// Token: 0x06000956 RID: 2390 RVA: 0x00020287 File Offset: 0x0001E687
	public static bool NormalTo(this Direction a, Direction b)
	{
		return !a.ParallelTo(b) && a != Direction.None && b != Direction.None;
	}

	// Token: 0x06000957 RID: 2391 RVA: 0x000202A6 File Offset: 0x0001E6A6
	public static bool Diagonal(this Direction d)
	{
		return d >= Direction.NorthEast && d < Direction.None;
	}

	// Token: 0x06000958 RID: 2392 RVA: 0x000202B6 File Offset: 0x0001E6B6
	public static bool Ortho(this Direction d)
	{
		return d < Direction.NorthEast;
	}

	// Token: 0x06000959 RID: 2393 RVA: 0x000202BC File Offset: 0x0001E6BC
	public static Direction ContinueRot(Direction from, Direction to)
	{
		return (Direction)DirectionUtil.continuerot[(int)to, (int)from];
	}

	// Token: 0x0600095A RID: 2394 RVA: 0x000202CA File Offset: 0x0001E6CA
	public static bool Valid(this Direction a)
	{
		return a != Direction.None;
	}

	// Token: 0x0600095B RID: 2395 RVA: 0x000202D3 File Offset: 0x0001E6D3
	public static bool Invalid(this Direction a)
	{
		return a == Direction.None;
	}

	// Token: 0x0600095C RID: 2396 RVA: 0x000202D9 File Offset: 0x0001E6D9
	public static Direction Inverse(this Direction d)
	{
		return (Direction)DirectionUtil.invrot[(int)d];
	}

	// Token: 0x0600095D RID: 2397 RVA: 0x000202E2 File Offset: 0x0001E6E2
	public static Direction RotClockwise90(this Direction d)
	{
		return (Direction)DirectionUtil.rotclockwise[(int)d];
	}

	// Token: 0x0600095E RID: 2398 RVA: 0x000202EB File Offset: 0x0001E6EB
	public static Direction FlipH(this Direction d)
	{
		return (Direction)DirectionUtil.fliph[(int)d];
	}

	// Token: 0x0600095F RID: 2399 RVA: 0x000202F4 File Offset: 0x0001E6F4
	public static Direction FlipV(this Direction d)
	{
		return (Direction)DirectionUtil.flipv[(int)d];
	}

	// Token: 0x06000960 RID: 2400 RVA: 0x000202FD File Offset: 0x0001E6FD
	public static Direction RotClockwise45(this Direction d)
	{
		return (Direction)DirectionUtil.rotclockwise45[(int)d];
	}

	// Token: 0x06000961 RID: 2401 RVA: 0x00020306 File Offset: 0x0001E706
	public static bool RotDir(Direction a, Direction b)
	{
		return a.LeftOf(b);
	}

	// Token: 0x06000962 RID: 2402 RVA: 0x00020317 File Offset: 0x0001E717
	public static Direction Rot90(this Direction d, bool clockwise)
	{
		if (clockwise)
		{
			return d.RotClockwise90();
		}
		return d.RotClockwise90().Inverse();
	}

	// Token: 0x06000963 RID: 2403 RVA: 0x00020331 File Offset: 0x0001E731
	public static Direction RotCounterclockwise90(this Direction d)
	{
		return (Direction)DirectionUtil.rotcounterclockwise[(int)d];
	}

	// Token: 0x06000964 RID: 2404 RVA: 0x0002033A File Offset: 0x0001E73A
	public static Direction RotCounterclockwise45(this Direction d)
	{
		return (Direction)DirectionUtil.rotcounterclockwise45[(int)d];
	}

	// Token: 0x06000965 RID: 2405 RVA: 0x00020344 File Offset: 0x0001E744
	public static Vector3 ToVector(this Direction d)
	{
		return d.ToCoord().DirectionToVector();
	}

	// Token: 0x06000966 RID: 2406 RVA: 0x00020360 File Offset: 0x0001E760
	public static Coord ToCoord(this Direction d)
	{
		if (d < Direction.North || d > (Direction)DirectionUtil.coordrep.Length)
		{
			string text = d.ToString();
			Debug.Log(text);
			Debug.Log("boop");
		}
		return DirectionUtil.coordrep[(int)d];
	}

	// Token: 0x06000967 RID: 2407 RVA: 0x000203B0 File Offset: 0x0001E7B0
	public static Direction FromNormal(Vector3 n)
	{
		n = n.normalized;
		float num = 10000f;
		int num2 = -1;
		for (int i = 0; i < DirectionUtil.coordrep.Length; i++)
		{
			float num3 = Vector3.Distance(DirectionUtil.coordrep[i].DirectionToVector(), n);
			if (num3 < num)
			{
				num = num3;
				num2 = i;
			}
		}
		return (Direction)num2;
	}

	// Token: 0x04000559 RID: 1369
	private static Quaternion[] dirrots = new Quaternion[]
	{
		Quaternion.Euler(270f, 180f, 0f),
		Quaternion.Euler(270f, 0f, 0f),
		Quaternion.Euler(270f, 90f, 0f),
		Quaternion.Euler(270f, 270f, 0f),
		Quaternion.Euler(270f, 225f, 0f),
		Quaternion.Euler(270f, 45f, 0f),
		Quaternion.Euler(270f, 135f, 0f),
		Quaternion.Euler(270f, 315f, 0f)
	};

	// Token: 0x0400055A RID: 1370
	private static int[,] crosses = new int[,]
	{
		{
			8, 8, 10, 9, 0, 0, 0, 0, 8, 0,
			0
		},
		{
			8, 8, 9, 10, 0, 0, 0, 0, 8, 0,
			0
		},
		{
			9, 10, 8, 8, 0, 0, 0, 0, 8, 0,
			0
		},
		{
			10, 9, 8, 8, 0, 0, 0, 0, 8, 0,
			0
		},
		{
			0, 0, 0, 0, 8, 0, 0, 0, 8, 0,
			0
		},
		{
			0, 0, 0, 0, 0, 8, 0, 0, 8, 0,
			0
		},
		{
			0, 0, 0, 0, 0, 0, 8, 0, 8, 0,
			0
		},
		{
			0, 0, 0, 0, 0, 0, 0, 8, 8, 0,
			0
		},
		{
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
			8
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0, 8, 8,
			8
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0, 8, 8,
			8
		}
	};

	// Token: 0x0400055B RID: 1371
	private static int[,] rotbetweens = new int[,]
	{
		{ -1, -1, 6, 4 },
		{ -1, -1, 5, 7 },
		{ 6, 5, -1, -1 },
		{ 4, 7, -1, -1 }
	};

	// Token: 0x0400055C RID: 1372
	private static int[,] continuerot = new int[,]
	{
		{ 8, 8, 8, 8, 6, 8, 4, 8 },
		{ 8, 8, 8, 8, 8, 7, 8, 5 },
		{ 8, 8, 8, 8, 8, 6, 5, 8 },
		{ 8, 8, 8, 8, 7, 8, 8, 4 },
		{ 3, 8, 8, 0, 8, 8, 8, 8 },
		{ 8, 2, 1, 8, 8, 8, 8, 8 },
		{ 2, 8, 0, 8, 8, 8, 8, 8 },
		{ 8, 3, 8, 1, 8, 8, 8, 8 }
	};

	// Token: 0x0400055D RID: 1373
	private static int[] invrot = new int[]
	{
		1, 0, 3, 2, 5, 4, 7, 6, 8, 10,
		9
	};

	// Token: 0x0400055E RID: 1374
	private static int[] rotclockwise = new int[]
	{
		3, 2, 0, 1, 7, 6, 4, 5, 8, 9,
		10
	};

	// Token: 0x0400055F RID: 1375
	private static int[] fliph = new int[]
	{
		0, 1, 3, 2, 6, 7, 4, 5, 8, 9,
		10
	};

	// Token: 0x04000560 RID: 1376
	private static int[] flipv = new int[]
	{
		1, 0, 2, 3, 7, 6, 5, 4, 8, 9,
		10
	};

	// Token: 0x04000561 RID: 1377
	private static int[] rotclockwise45 = new int[]
	{
		4, 5, 7, 6, 2, 3, 0, 1, 8, 9,
		10
	};

	// Token: 0x04000562 RID: 1378
	private static int[] rotcounterclockwise = new int[]
	{
		2, 3, 1, 0, 6, 7, 5, 4, 8, 9,
		10
	};

	// Token: 0x04000563 RID: 1379
	private static int[] rotcounterclockwise45 = new int[]
	{
		6, 7, 4, 5, 0, 1, 3, 2, 8, 9,
		10
	};

	// Token: 0x04000564 RID: 1380
	private static Coord[] coordrep = new Coord[]
	{
		new Coord(0, -1, 0),
		new Coord(0, 1, 0),
		new Coord(-1, 0, 0),
		new Coord(1, 0, 0),
		new Coord(1, -1, 0),
		new Coord(-1, 1, 0),
		new Coord(-1, -1, 0),
		new Coord(1, 1, 0),
		new Coord(0, 0, 0),
		new Coord(0, 0, -1),
		new Coord(0, 0, 1)
	};
}
