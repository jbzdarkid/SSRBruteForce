using System;
using UnityEngine;

// Token: 0x02000098 RID: 152
public struct Coord
{
	// Token: 0x06000934 RID: 2356 RVA: 0x0001FC74 File Offset: 0x0001E074
	public Coord(int _x, int _y, int _z = 0)
	{
		this.x = _x;
		this.y = _y;
		this.z = _z;
	}

	// Token: 0x06000935 RID: 2357 RVA: 0x0001FC8B File Offset: 0x0001E08B
	public Coord FlipH()
	{
		return new Coord(-this.x, this.y, this.z);
	}

	// Token: 0x06000936 RID: 2358 RVA: 0x0001FCA5 File Offset: 0x0001E0A5
	public Coord FlipV()
	{
		return new Coord(this.x, -this.y, this.z);
	}

	// Token: 0x06000937 RID: 2359 RVA: 0x0001FCBF File Offset: 0x0001E0BF
	public Coord RotateClockwise()
	{
		return new Coord(-this.y, this.x, this.z);
	}

	// Token: 0x06000938 RID: 2360 RVA: 0x0001FCD9 File Offset: 0x0001E0D9
	public static int Dot(Coord a, Coord b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	// Token: 0x06000939 RID: 2361 RVA: 0x0001FD0C File Offset: 0x0001E10C
	public override string ToString()
	{
		return string.Concat(new string[]
		{
			"(",
			this.x.ToStringFast(),
			",",
			this.y.ToStringFast(),
			",",
			this.z.ToStringFast(),
			")"
		});
	}

	// Token: 0x0600093A RID: 2362 RVA: 0x0001FD6E File Offset: 0x0001E16E
	public bool Above(Coord other)
	{
		return this.x == other.x && this.y == other.y && this.z >= other.z;
	}

	// Token: 0x0600093B RID: 2363 RVA: 0x0001FDA9 File Offset: 0x0001E1A9
	public override int GetHashCode()
	{
		return this.x + this.y * 100 + this.z * 10000;
	}

	// Token: 0x0600093C RID: 2364 RVA: 0x0001FDC8 File Offset: 0x0001E1C8
	public override bool Equals(object obj)
	{
		if (obj == null || base.GetType() != obj.GetType())
		{
			return false;
		}
		Coord coord = (Coord)obj;
		return this.x == coord.x && this.y == coord.y && this.z == coord.z;
	}

	// Token: 0x0600093D RID: 2365 RVA: 0x0001FE38 File Offset: 0x0001E238
	public int AmountInDirection(Direction dir)
	{
		switch (dir)
		{
		case Direction.North:
			return -this.y;
		case Direction.South:
			return this.y;
		case Direction.West:
			return -this.x;
		case Direction.East:
			return this.x;
		case Direction.Down:
			return this.z;
		case Direction.Up:
			return -this.z;
		}
		Debug.LogError("eep " + dir);
		return 0;
	}

	// Token: 0x0600093E RID: 2366 RVA: 0x0001FEC4 File Offset: 0x0001E2C4
	public static Coord operator +(Coord a, Coord b)
	{
		return new Coord(a.x + b.x, a.y + b.y, a.z + b.z);
	}

	// Token: 0x0600093F RID: 2367 RVA: 0x0001FEF8 File Offset: 0x0001E2F8
	public static Coord operator +(Coord a, Direction b)
	{
		return a + b.ToCoord();
	}

	// Token: 0x06000940 RID: 2368 RVA: 0x0001FF06 File Offset: 0x0001E306
	public static Coord operator -(Coord a, Direction b)
	{
		return a - b.ToCoord();
	}

	// Token: 0x06000941 RID: 2369 RVA: 0x0001FF14 File Offset: 0x0001E314
	public static Coord operator -(Coord a, Coord b)
	{
		return new Coord(a.x - b.x, a.y - b.y, a.z - b.z);
	}

	// Token: 0x06000942 RID: 2370 RVA: 0x0001FF48 File Offset: 0x0001E348
	public static Coord operator *(int n, Coord b)
	{
		return new Coord(n * b.x, n * b.y, n * b.z);
	}

	// Token: 0x06000943 RID: 2371 RVA: 0x0001FF6A File Offset: 0x0001E36A
	public static Coord operator /(Coord a, int b)
	{
		return new Coord(a.x / b, a.y / b, a.z / b);
	}

	// Token: 0x06000944 RID: 2372 RVA: 0x0001FF8C File Offset: 0x0001E38C
	public static bool operator ==(Coord a, Coord b)
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	// Token: 0x06000945 RID: 2373 RVA: 0x0001FFC7 File Offset: 0x0001E3C7
	public static bool operator !=(Coord a, Coord b)
	{
		return !(a == b);
	}

	// Token: 0x06000946 RID: 2374 RVA: 0x0001FFD4 File Offset: 0x0001E3D4
	public Vector3 ToVector(bool pure = false)
	{
		if (pure)
		{
			return new Vector3((float)this.x, (float)this.z, (float)(-(float)this.y));
		}
		return new Vector3((float)this.x + 0.5f, (float)this.z, (float)(-(float)this.y) - 0.5f);
	}

	// Token: 0x06000947 RID: 2375 RVA: 0x0002002C File Offset: 0x0001E42C
	public static int DistanceSq(Coord a, Coord b)
	{
		return (b - a).MagnitudeSq();
	}

	// Token: 0x06000948 RID: 2376 RVA: 0x00020048 File Offset: 0x0001E448
	public static float Distance(Coord a, Coord b)
	{
		return (b - a).Magnitude();
	}

	// Token: 0x06000949 RID: 2377 RVA: 0x00020064 File Offset: 0x0001E464
	public int MagnitudeSq()
	{
		return this.x * this.x + this.y * this.y + this.z * this.z;
	}

	// Token: 0x0600094A RID: 2378 RVA: 0x0002008F File Offset: 0x0001E48F
	public float Magnitude()
	{
		return (float)Math.Sqrt((double)(this.x * this.x + this.y * this.y + this.z * this.z));
	}

	// Token: 0x0600094B RID: 2379 RVA: 0x000200C4 File Offset: 0x0001E4C4
	public Vector3 DirectionToVector()
	{
		Vector3 vector = new Vector3(1f, 0f, 0f);
		Vector3 vector2 = new Vector3(0f, 0f, -1f);
		Vector3 vector3 = new Vector3(0f, 1f, 0f);
		return ((float)this.x * vector + (float)this.y * vector2 + (float)this.z * vector3).normalized;
	}

	// Token: 0x04000541 RID: 1345
	public readonly int x;

	// Token: 0x04000542 RID: 1346
	public readonly int y;

	// Token: 0x04000543 RID: 1347
	public readonly int z;

	// Token: 0x04000544 RID: 1348
	public static Coord North = new Coord(0, 1, 0);

	// Token: 0x04000545 RID: 1349
	public static Coord South = new Coord(0, -1, 0);

	// Token: 0x04000546 RID: 1350
	public static Coord East = new Coord(1, 0, 0);

	// Token: 0x04000547 RID: 1351
	public static Coord West = new Coord(-1, 0, 0);

	// Token: 0x04000548 RID: 1352
	public static Coord Up = new Coord(0, 0, 1);

	// Token: 0x04000549 RID: 1353
	public static Coord Down = new Coord(0, 0, -1);

	// Token: 0x0400054A RID: 1354
	public static Coord Zero = new Coord(0, 0, 0);

	// Token: 0x0400054B RID: 1355
	public static Coord One = new Coord(1, 1, 1);

	// Token: 0x0400054C RID: 1356
	public static Coord Invalid = new Coord(int.MinValue, int.MinValue, int.MinValue);
}
