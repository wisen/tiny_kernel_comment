int LowerImageLimit[32][32] = {
	{1050, 1052, 1053, 1056, 1058, 1058, 1061, 1065, 1063, 1071, 1076, 1081, 1100, 1150, 1297,
	 912, 913, 914, 915, 915, 916, 917, 919, 920, 922, 923, 925, 935, 904, 300, 300, 300},
	{962, 980, 981, 982, 982, 983, 983, 983, 981, 985, 991, 989, 983, 985, 989, 959, 961, 961,
	 962, 963, 964, 965, 966, 967, 968, 970, 971, 972, 945, 300, 300, 300},
	{957, 976, 977, 977, 977, 979, 979, 979, 979, 982, 983, 980, 980, 980, 992, 959, 961, 962,
	 963, 963, 965, 965, 966, 968, 969, 970, 980, 972, 947, 300, 300, 300},
	{954, 972, 973, 973, 974, 975, 975, 975, 983, 976, 974, 975, 976, 977, 979, 960, 962, 962,
	 963, 963, 965, 966, 966, 968, 970, 971, 972, 974, 948, 300, 300, 300},
	{951, 970, 971, 980, 971, 972, 972, 972, 973, 972, 975, 972, 972, 972, 973, 960, 962, 963,
	 963, 964, 965, 966, 967, 968, 970, 972, 973, 975, 950, 300, 300, 300},
	{958, 968, 969, 970, 969, 970, 971, 970, 969, 970, 969, 970, 970, 970, 971, 961, 963, 964,
	 965, 965, 966, 967, 968, 969, 971, 973, 974, 977, 953, 300, 300, 300},
	{947, 966, 967, 967, 967, 977, 969, 968, 967, 968, 967, 967, 968, 968, 969, 962, 964, 965,
	 965, 965, 967, 967, 969, 970, 972, 974, 976, 979, 955, 300, 300, 300},
	{943, 963, 963, 963, 973, 965, 965, 965, 964, 964, 964, 964, 965, 965, 965, 962, 964, 965,
	 966, 966, 968, 969, 970, 971, 973, 975, 976, 980, 958, 300, 300, 300},
	{942, 962, 962, 963, 963, 964, 965, 964, 964, 964, 1001, 964, 964, 973, 965, 964, 966, 966,
	 967, 968, 969, 970, 971, 973, 975, 979, 979, 984, 965, 300, 300, 300},
	{940, 960, 969, 961, 961, 962, 962, 962, 961, 962, 961, 962, 963, 962, 963, 966, 967, 968,
	 969, 970, 971, 972, 974, 975, 977, 979, 980, 984, 962, 300, 300, 300},
	{938, 958, 958, 959, 959, 960, 960, 969, 959, 960, 969, 960, 960, 961, 961, 968, 969, 970,
	 971, 971, 973, 973, 974, 976, 978, 979, 981, 984, 962, 300, 300, 300},
	{937, 956, 957, 957, 958, 958, 968, 959, 957, 958, 958, 959, 960, 960, 960, 969, 971, 971,
	 972, 972, 974, 975, 976, 977, 979, 981, 983, 985, 961, 300, 300, 300},
	{936, 955, 955, 956, 957, 958, 957, 957, 956, 957, 959, 967, 958, 958, 959, 971, 972, 973,
	 974, 974, 976, 976, 977, 979, 981, 982, 984, 986, 961, 300, 300, 300},
	{935, 954, 954, 955, 955, 956, 956, 956, 955, 957, 956, 957, 958, 958, 967, 973, 975, 975,
	 975, 976, 977, 978, 979, 981, 983, 984, 986, 987, 964, 300, 300, 300},
	{934, 952, 953, 953, 954, 955, 955, 956, 954, 964, 960, 956, 957, 957, 957, 976, 977, 977,
	 978, 978, 979, 980, 981, 982, 984, 986, 987, 991, 983, 300, 300, 300},
	{929, 947, 947, 948, 949, 949, 950, 950, 949, 950, 949, 951, 961, 952, 951, 1026, 1016,
	 1015, 1013, 1013, 1013, 1011, 1011, 1010, 1010, 1009, 1008, 1006, 984, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300},
	{300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300,
	 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300, 300}
};

int UpperImageLimit[32][32] = {
	{1951, 1954, 1955, 1960, 1965, 1964, 1971, 1978, 1973, 1989, 1998, 2008, 2043, 2136, 2409,
	 1693, 1696, 1698, 1699, 1700, 1702, 1704, 1706, 1709, 1712, 1715, 1717, 1737, 1679, 5500,
	 5500, 5500},
	{1787, 1821, 1821, 1823, 1824, 1825, 1825, 1825, 1822, 1828, 1840, 1836, 1826, 1829, 1836,
	 1781, 1784, 1786, 1786, 1788, 1790, 1792, 1794, 1796, 1799, 1801, 1804, 1805, 1756, 5500,
	 5500, 5500},
	{1777, 1812, 1814, 1815, 1815, 1817, 1818, 1819, 1818, 1823, 1825, 1819, 1820, 1821, 1842,
	 1782, 1786, 1787, 1788, 1789, 1792, 1792, 1795, 1797, 1800, 1802, 1820, 1806, 1758, 5500,
	 5500, 5500},
	{1771, 1805, 1807, 1808, 1808, 1810, 1811, 1811, 1825, 1813, 1809, 1811, 1812, 1815, 1819,
	 1782, 1786, 1787, 1788, 1789, 1792, 1793, 1795, 1797, 1801, 1803, 1805, 1808, 1760, 5500,
	 5500, 5500},
	{1767, 1802, 1803, 1821, 1804, 1806, 1806, 1806, 1808, 1805, 1811, 1805, 1805, 1806, 1806,
	 1783, 1786, 1788, 1789, 1790, 1793, 1794, 1796, 1799, 1802, 1804, 1807, 1810, 1764, 5500,
	 5500, 5500},
	{1779, 1798, 1799, 1801, 1800, 1802, 1803, 1802, 1800, 1801, 1800, 1801, 1801, 1801, 1803,
	 1785, 1788, 1790, 1791, 1791, 1794, 1796, 1798, 1800, 1803, 1807, 1809, 1814, 1770, 5500,
	 5500, 5500},
	{1758, 1793, 1795, 1796, 1796, 1814, 1799, 1798, 1796, 1797, 1795, 1796, 1797, 1798, 1799,
	 1787, 1790, 1791, 1792, 1793, 1795, 1797, 1799, 1801, 1805, 1808, 1812, 1817, 1774, 5500,
	 5500, 5500},
	{1752, 1788, 1788, 1789, 1807, 1792, 1793, 1792, 1790, 1791, 1789, 1791, 1792, 1792, 1793,
	 1786, 1790, 1792, 1794, 1794, 1797, 1799, 1801, 1804, 1807, 1810, 1813, 1820, 1779, 5500,
	 5500, 5500},
	{1750, 1786, 1787, 1789, 1789, 1790, 1791, 1791, 1790, 1790, 1859, 1790, 1790, 1808, 1792,
	 1791, 1794, 1795, 1795, 1797, 1800, 1802, 1803, 1806, 1811, 1818, 1819, 1827, 1792, 5500,
	 5500, 5500},
	{1746, 1783, 1800, 1785, 1785, 1787, 1787, 1787, 1786, 1786, 1785, 1787, 1788, 1787, 1789,
	 1794, 1796, 1798, 1799, 1801, 1804, 1805, 1809, 1811, 1815, 1818, 1821, 1827, 1787, 5500,
	 5500, 5500},
	{1742, 1779, 1780, 1781, 1782, 1783, 1783, 1799, 1781, 1783, 1799, 1783, 1783, 1785, 1785,
	 1798, 1800, 1801, 1803, 1803, 1806, 1807, 1809, 1812, 1816, 1819, 1822, 1828, 1787, 5500,
	 5500, 5500},
	{1740, 1776, 1777, 1778, 1778, 1780, 1797, 1780, 1778, 1780, 1779, 1780, 1782, 1782, 1783,
	 1800, 1802, 1804, 1805, 1806, 1808, 1810, 1812, 1815, 1819, 1822, 1826, 1829, 1785, 5500,
	 5500, 5500},
	{1738, 1774, 1773, 1776, 1777, 1779, 1778, 1778, 1776, 1778, 1781, 1795, 1780, 1780, 1782,
	 1804, 1805, 1807, 1808, 1809, 1812, 1813, 1815, 1818, 1821, 1825, 1828, 1832, 1786, 5500,
	 5500, 5500},
	{1736, 1772, 1771, 1773, 1774, 1775, 1776, 1776, 1774, 1777, 1775, 1778, 1779, 1779, 1795,
	 1808, 1810, 1810, 1811, 1812, 1815, 1817, 1819, 1822, 1825, 1827, 1830, 1833, 1789, 5500,
	 5500, 5500},
	{1734, 1769, 1770, 1771, 1771, 1773, 1774, 1775, 1772, 1791, 1784, 1775, 1777, 1777, 1777,
	 1812, 1814, 1815, 1816, 1816, 1819, 1821, 1823, 1825, 1828, 1831, 1833, 1840, 1825, 5500,
	 5500, 5500},
	{1725, 1759, 1760, 1761, 1762, 1763, 1764, 1764, 1762, 1765, 1763, 1767, 1784, 1768, 1765,
	 1906, 1887, 1885, 1882, 1881, 1881, 1878, 1878, 1876, 1876, 1873, 1871, 1869, 1828, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500},
	{5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
	 5500, 5500}
};

int SensorSpeedLowerImageLimit[32][32] = {
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500},
	{-500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500, -500,
	 -500, -500}
};

int SensorSpeedUpperImageLimit[32][32] = {
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500},
	{500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500}
};

int ADCLowerImageLimit[32][32] = {
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25},
	{25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	 25, 25, 25, 25, 25, 25, 25, 25, 25}
};

int ADCUpperImageLimit[32][32] = {
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230},
	{230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230,
	 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230}
};
