using System;
using System.Collections.Generic;
using UnityEngine;

// Token: 0x0200011D RID: 285
public static class Utility
{
	// (IsVisibleFrom removed: graphics-only frustum cull, not needed headlessly)

	// Token: 0x06000E22 RID: 3618 RVA: 0x0005B23C File Offset: 0x0005963C
	public static T[] RemoveAt<T>(this T[] source, int index)
	{
		T[] array = new T[source.Length - 1];
		if (index > 0)
		{
			Array.Copy(source, 0, array, 0, index);
		}
		if (index < source.Length - 1)
		{
			Array.Copy(source, index + 1, array, index, source.Length - index - 1);
		}
		return array;
	}

	// Token: 0x06000E23 RID: 3619 RVA: 0x0005B284 File Offset: 0x00059684
	public static T[] Remove<T>(this T[] source, T item)
	{
		for (int i = 0; i < source.Length; i++)
		{
			if (source[i].Equals(item))
			{
				return source.RemoveAt<T>(i);
			}
		}
		return source;
	}

	// Token: 0x06000E24 RID: 3620 RVA: 0x0005B2CD File Offset: 0x000596CD
	public static T Random<T>(this T[] ar)
	{
		return ar[global::UnityEngine.Random.Range(0, ar.Length)];
	}

	// Token: 0x06000E25 RID: 3621 RVA: 0x0005B2DE File Offset: 0x000596DE
	public static T Random<T>(this List<T> ar)
	{
		return ar[global::UnityEngine.Random.Range(0, ar.Count)];
	}

	// Token: 0x06000E26 RID: 3622 RVA: 0x0005B2F2 File Offset: 0x000596F2
	public static bool AddUnique<T>(this List<T> list, T elem)
	{
		if (list.IndexOf(elem) == -1)
		{
			list.Add(elem);
			return true;
		}
		return false;
	}

	// Token: 0x06000E27 RID: 3623 RVA: 0x0005B30B File Offset: 0x0005970B
	public static bool Emtpy<T>(this List<T> list)
	{
		return list.Count == 0;
	}

	// Token: 0x06000E28 RID: 3624 RVA: 0x0005B316 File Offset: 0x00059716
	public static bool Directional(this EntType e)
	{
		return e == EntType.player || e == EntType.ladder || e == EntType.sausage;
	}

	// Token: 0x06000E29 RID: 3625 RVA: 0x0005B330 File Offset: 0x00059730
	public static bool Solid(this Entity e)
	{
		return !e.Decoration();
	}

	// Token: 0x06000E2A RID: 3626 RVA: 0x0005B33B File Offset: 0x0005973B
	public static bool CanHatTurn(this EntType et)
	{
		return et == EntType.sausage || et == EntType.fork;
	}

	// Token: 0x06000E2B RID: 3627 RVA: 0x0005B34B File Offset: 0x0005974B
	public static bool SubjectToPassiveForces(this EntType et)
	{
		return et == EntType.sausage || et == EntType.fork || et == EntType.island;
	}

	// Token: 0x06000E2C RID: 3628 RVA: 0x0005B362 File Offset: 0x00059762
	public static bool NeedsGround(this EntType e)
	{
		return e == EntType.player || e == EntType.sausage || e == EntType.fork;
	}

	// Token: 0x06000E2D RID: 3629 RVA: 0x0005B37C File Offset: 0x0005977C
	public static Vector3 Round(this Vector3 v)
	{
		return new Vector3((float)Math.Round((double)v.x), (float)Math.Round((double)v.y), (float)Math.Round((double)v.z));
	}

	// Token: 0x06000E2E RID: 3630 RVA: 0x0005B3AD File Offset: 0x000597AD
	public static Vector3 Floor(this Vector3 v)
	{
		return new Vector3(Mathf.Floor(v.x), Mathf.Floor(v.y), Mathf.Floor(v.z));
	}

	// Token: 0x06000E2F RID: 3631 RVA: 0x0005B3D8 File Offset: 0x000597D8
	public static Coord RoundToCoord(this Vector3 v)
	{
		return new Coord(Mathf.RoundToInt(v.x), -Mathf.RoundToInt(v.z), Mathf.RoundToInt(v.y));
	}

	// Token: 0x06000E30 RID: 3632 RVA: 0x0005B404 File Offset: 0x00059804
	public static bool Dynamic(this EntType e)
	{
		return !e.Static();
	}

	// Token: 0x06000E31 RID: 3633 RVA: 0x0005B40F File Offset: 0x0005980F
	public static bool Static(this EntType e)
	{
		return e != EntType.player && e != EntType.sausage && e != EntType.barrier && e != EntType.fork && e != EntType.island;
	}

	// Token: 0x06000E32 RID: 3634 RVA: 0x0005B437 File Offset: 0x00059837
	public static bool CanRoll(this EntType e)
	{
		return e == EntType.sausage;
	}
}
