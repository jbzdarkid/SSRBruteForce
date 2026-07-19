using System;

// Token: 0x02000097 RID: 151
public struct BoundingBox
{
	// Token: 0x0600092D RID: 2349 RVA: 0x0001F7CC File Offset: 0x0001DBCC
	public BoundingBox(Coord[] coords)
	{
		int num = 1000000;
		int num2 = 1000000;
		int num3 = 1000000;
		int num4 = -1000000;
		int num5 = -1000000;
		int num6 = -1000000;
		foreach (Coord coord in coords)
		{
			if (coord.x < num)
			{
				num = coord.x;
			}
			if (coord.y < num2)
			{
				num2 = coord.y;
			}
			if (coord.z < num3)
			{
				num3 = coord.z;
			}
			if (coord.x > num4)
			{
				num4 = coord.x;
			}
			if (coord.y > num5)
			{
				num5 = coord.y;
			}
			if (coord.z > num6)
			{
				num6 = coord.z;
			}
		}
		this.id = -1;
		this.min = new Coord(num, num2, num3);
		this.max = new Coord(num4, num5, num6);
	}

	// Token: 0x0600092E RID: 2350 RVA: 0x0001F8D2 File Offset: 0x0001DCD2
	public BoundingBox(Coord min, Coord max, int id)
	{
		this.min = min;
		this.max = max;
		this.id = id;
	}

	// Token: 0x0600092F RID: 2351 RVA: 0x0001F8EC File Offset: 0x0001DCEC
	public static bool Overlaps(BoundingBox a, BoundingBox b)
	{
		return a.min.x <= b.max.x && a.min.y <= b.max.y && a.min.z <= b.max.z && b.min.x <= a.max.x && b.min.y <= a.max.y && b.min.z <= a.max.z;
	}

	// Token: 0x06000930 RID: 2352 RVA: 0x0001F9D8 File Offset: 0x0001DDD8
	public bool Overlaps(BoundingBox other)
	{
		return other.min.x <= this.max.x && other.min.y <= this.max.y && other.min.z <= this.max.z && this.min.x <= other.max.x && this.min.y <= other.max.y && this.min.z <= other.max.z;
	}

	// Token: 0x06000931 RID: 2353 RVA: 0x0001FABC File Offset: 0x0001DEBC
	public bool Overlaps(Coord other)
	{
		return other.x <= this.max.x && other.y <= this.max.y && other.z <= this.max.z && this.min.x <= other.x && this.min.y <= other.y && this.min.z <= other.z;
	}

	// Token: 0x06000932 RID: 2354 RVA: 0x0001FB6C File Offset: 0x0001DF6C
	public BoundingBox Intersect(BoundingBox other)
	{
		return new BoundingBox(new Coord(Math.Max(this.min.x, other.min.x), Math.Max(this.min.y, other.min.y), Math.Max(this.min.z, other.min.z)), new Coord(Math.Min(this.max.x, other.max.x), Math.Min(this.max.y, other.max.y), Math.Min(this.max.z, other.max.z)), -1);
	}

	// Token: 0x0400053D RID: 1341
	public static BoundingBox Invalid = new BoundingBox(Coord.Invalid, Coord.Invalid, -1);

	// Token: 0x0400053E RID: 1342
	public readonly Coord min;

	// Token: 0x0400053F RID: 1343
	public readonly Coord max;

	// Token: 0x04000540 RID: 1344
	public readonly int id;
}
