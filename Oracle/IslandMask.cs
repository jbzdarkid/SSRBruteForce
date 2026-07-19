using System;
using System.Linq;

// Token: 0x020000A4 RID: 164
public class IslandMask
{
	// Token: 0x06000A96 RID: 2710 RVA: 0x00031825 File Offset: 0x0002FC25
	public IslandMask(int[][][] mask, Coord offset)
	{
		this.mask = mask;
		this.offset = offset;
	}

	// Token: 0x06000A97 RID: 2711 RVA: 0x0003183C File Offset: 0x0002FC3C
	public override string ToString()
	{
		int num = this.mask.Length;
		int num2 = ((num != 0) ? this.mask[0].Length : 0);
		int num3 = ((num2 != 0) ? this.mask[0][0].Length : 0);
		string text = string.Concat(new object[]
		{
			this.offset.x,
			",",
			this.offset.y,
			",",
			this.offset.z,
			",",
			num,
			",",
			num2,
			",",
			num3,
			","
		});
		for (int i = 0; i < num; i++)
		{
			for (int j = 0; j < num2; j++)
			{
				for (int k = 0; k < num3; k++)
				{
					text = text + this.mask[i][j][k] + ",";
				}
			}
		}
		return text;
	}

	// Token: 0x06000A98 RID: 2712 RVA: 0x0003197C File Offset: 0x0002FD7C
	public static IslandMask FromString(string s)
	{
		int[] array = s.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries).Select<string, int>(new Func<string, int>(ParseUtils.IntParseFast)).ToArray<int>();
		Coord coord = new Coord(array[0], array[1], array[2]);
		int num = array[3];
		int num2 = array[4];
		int num3 = array[5];
		int[][][] array2 = new int[num][][];
		for (int i = 0; i < num; i++)
		{
			array2[i] = new int[num2][];
			for (int j = 0; j < num2; j++)
			{
				array2[i][j] = new int[num3];
				for (int k = 0; k < num3; k++)
				{
					int num4 = k + num3 * j + num2 * num3 * i;
					array2[i][j][k] = array[6 + num4];
				}
			}
		}
		return new IslandMask(array2, coord);
	}

	// Token: 0x06000A99 RID: 2713 RVA: 0x00031A70 File Offset: 0x0002FE70
	public IslandMask Projection()
	{
		int num = this.mask.Length;
		int num2 = ((num != 0) ? this.mask[0].Length : 0);
		int num3 = ((num2 != 0) ? this.mask[0][0].Length : 0);
		int[][][] array = new int[num][][];
		for (int i = 0; i < num; i++)
		{
			array[i] = new int[num2][];
			for (int j = 0; j < num2; j++)
			{
				array[i][j] = new int[1];
				for (int k = 0; k < num3; k++)
				{
					if (this.mask[i][j][k] != 0)
					{
						array[i][j][0] = 1;
						break;
					}
				}
			}
		}
		return new IslandMask(array, this.offset);
	}

	// Token: 0x04000681 RID: 1665
	public int[][][] mask;

	// Token: 0x04000682 RID: 1666
	public Coord offset;
}
