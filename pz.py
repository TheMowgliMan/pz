from collections import deque, Counter
from time import time

from findbetween import findbetween

import copy

def timestamp():
    return time() * 1000

class PzBinTI:
    def __init__(self, data, left=None, right=None, center=None):
        self.data = data
        self.left = left
        self.right = right
        self.center = center

    def get(self, num):
        match num:
            case 0:
                return self.data
            case 1:
                return self.left
            case 2:
                return self.right
            case 3:
                return self.center
            case _:
                raise IndexError("PzBinTI.get() only takes values 0-3\n")

class PzBinT:
    def __init__(self):
        self.head = None
        self.data_series = [] # Used for quickly writing out the tree as a list. This relies on the fact that add() always adds nodes in a certain order

        self.fastfind = {}

    def add(self, data):
        item = PzBinTI(data)

        self.data_series.append(data)

        if self.head == None:
            self.head = item
        else:
            t_head = self.head
            q = deque([t_head])

            while q:
                t = q.popleft()

                if t.get(1):
                    q.append(t.left)
                else:
                    t.left = PzBinTI(data)
                    return

                if t.get(2):
                    q.append(t.right)
                else:
                    t.right = PzBinTI(data)
                    return

                if t.get(3):
                    q.append(t.center)
                else:
                    t.center = PzBinTI(data)
                    return
            self.head = t_head

        iof = self.in_order_find(data)
        len_iof = len(iof)
        hashed = sum([(iof[i] << (2 * (len_iof - i - 1))) for i in range(len_iof - 1, -1, -1)])
        self.fastfind[hashed] = data

    def export(self):
        return self.data_series

    def __iop(data):
        print(data.data)
        if data.get(1):
            PzBinT.__iop(data.get(1))
        if data.get(2):
            PzBinT.__iop(data.get(2))
        if data.get(3):
            PzBinT.__iop(data.get(3))

    def in_order_print(self):
        PzBinT.__iop(self.head)

    def __iof(data, match, chain):
        if data.data == match:
            chain.append(0)
            return chain

        if data.get(1):
            t = copy.deepcopy(chain) + [1]
            r = PzBinT.__iof(data.get(1), match, t)
            if r:
                return r
        if data.get(2):
            t = copy.deepcopy(chain) + [2]
            r = PzBinT.__iof(data.get(2), match, t)
            if r:
                return r
        if data.get(3):
            t = copy.deepcopy(chain) + [3]
            r = PzBinT.__iof(data.get(3), match, t)
            if r:
                return r

    def in_order_find(self, match):
        chain = []
        return PzBinT.__iof(self.head, match, chain)

    def get(self, item:list):
        len_item = len(item)
        hashed = sum([(item[i] << (2 * (len_item - i - 1))) for i in range(len_item - 1, -1, -1)])
        not_in = False
        if hashed in self.fastfind:
            return self.fastfind[hashed]
        else:
            not_in = True

        link = self.head
        for i in item:
            if i == 0:
                if not_in:
                    self.fastfind[hashed] = link.data
                return link.data
            else:
                link = link.get(i)

class PzCompressor:
    def __init__(self):
        self.tree = None
        self.symbols = {}
        self.fastfind = {}

    def __add_symbol(self, symbol):
        self.tree.add(symbol)
        self.symbols[symbol] = self.tree.in_order_find(symbol)

    def __add_all_symbols(self):
        self.__add_symbol("[REPT]")
        self.__add_symbol("[BACKREF]")
        self.__add_symbol("[PZ META]")
        self.__add_symbol("[END META]")
        self.__add_symbol("[PZ PROTOCOL 0]")

    def __export(self):
        return "\n".join([str(x) for x in self.tree.export()])

    def export_tree(self, fname:str):
        to_write = self.__export()

        with open(fname, "w") as extree:
            extree.write(to_write)

    def __import(self, tree):
        self.tree = PzBinT()
        self.symbols = {}
        for line in tree:
            line = line.rstrip()

            if line.isdigit():
                self.tree.add(int(line))
            else:
                self.tree.add(line)

            if line[0] == "[":
                self.symbols[line] = self.tree.in_order_find(line)

    def import_tree(self, fname:str):
        self.tree = PzBinT()
        self.symbols = {}
        with open(fname) as tree:
            self.__import(tree)

    def get_sym(self, sym):
        return self.symbols["[" + sym + "]"]

    def generate_tree(self, data):
        if self.tree == None:
            self.tree = PzBinT()

        data = bytearray(data)

        data.extend(bytes(range(256)))
        data = self.__binarize(bytes(data))

        c = Counter(data)

        firstcount = None
        symbols_not_added = True

        for char,count in c.most_common():
            if firstcount == None:
                firstcount = count

            # print(char, count)

            if count < (firstcount / 100) and symbols_not_added: # Yayy magic number
                self.__add_all_symbols()
                symbols_not_added = False

            self.tree.add(char)
            self.fastfind[char] = self.tree.in_order_find(char)

        if symbols_not_added:
            self.__add_all_symbols()

        self.__add_symbol("[EOA]")

    def __binarize(self, data):
        if isinstance(data, str):
            return bytes(data, "utf-8")
        elif isinstance(data, bytes):
            return data
        else:
            print(data, type(data))
            raise TypeError("Improper datatype")

    def __generate_tree_for_insertion(self):
        t = self.__export()
        return (len(t), [PzBinUtil.kaboom_char(byte) for byte in t.encode("utf-8")])

    def compress(self, data):
        print("Generating tree...")
        self.generate_tree(data)

        data = self.__binarize(data)

        print("Compressing data...")

        r = []
        last_char = ""
        last_char_count = 1

        ref_q = deque(maxlen=2048)
        ex_q = deque()
        ex_q_sz = 0

        for bc in data:
            ex_q.append(bc)
            ex_q_sz += 1

            if ex_q_sz > 255:
                found = findbetween(ref_q, ex_q)
                if found[0] >= 0 and (len(ref_q) - found[0]) > 255:
                    r.append(self.get_sym("BACKREF"))

                    integer = PzBinUtil.kaboom_short(65535 - found[0])
                    integer.append(0)
                    r.append(integer)

                    integer = PzBinUtil.kaboom_char(found[1])
                    integer.append(0)
                    r.append(integer)

                    for i in range(found[1]):
                        ex_q.popleft()
                else:
                    c = ex_q.popleft()
                    ex_q_sz -= 1

                    if not c in self.fastfind:
                        self.tree.add(c)
                        self.fastfind[c] = self.tree.in_order_find(c)

                    if last_char != c or last_char_count == 255:
                        if last_char != "":
                            if last_char_count == 1:
                                r.append(self.fastfind[last_char])
                                last_char = c
                            else:
                                if last_char_count > 2:
                                    r.append(self.get_sym("REPT"))

                                    integer = PzBinUtil.kaboom_char(last_char_count)
                                    integer.append(0)
                                    r.append(integer)

                                    r.append(self.fastfind[last_char])

                                    last_char = c
                                    last_char_count = 1
                                else:
                                    for i in range(last_char_count):
                                        r.append(self.fastfind[last_char])

                                    last_char = c
                                    last_char_count = 1
                        else:
                            last_char = c
                            last_char_count = 1
                    elif last_char == c and last_char_count < 255:
                        last_char_count += 1
                    else:
                        print("Wrong clause reached")

            ref_q.append(bc)

        if last_char_count == 1:
            r.append(self.fastfind[last_char])
            last_char = c
        else:
            r.append(self.get_sym("REPT"))
            integer = PzBinUtil.kaboom_char(last_char_count)
            integer.append(0)
            r.append(integer)
            r.append(self.fastfind[last_char])

        if len(ex_q) > 0:
            for c in ex_q:
                r.append(self.fastfind[c])

        tree = self.__generate_tree_for_insertion()

        meta = [PzBinUtil.kaboom_dword(tree[0])]
        meta.extend(tree[1])

        meta.append(self.get_sym("PZ META"))
        meta.append(self.get_sym("PZ PROTOCOL 0"))
        meta.append(self.get_sym("END META"))

        r = meta + r

        r.append(self.get_sym("EOA"))

        return PzBinUtil.to_binary(r)

    def decompress(self, data:bytes):
        t = timestamp()
        print("Retrieving tree@", timestamp() - t)
        tree_size = data[3] + data[2] * 256 + data[1] * 65536 + data[0] * 16777216

        tree_string = bytes(data[4 : tree_size + 4])
        tree_string = tree_string.decode("utf-8")

        print("Formatting tree@", timestamp() - t)

        tree_list = tree_string.split("\n")

        # tree_bin = bytes(data[4:tree_size])
        # tree_list = []
        # for i in tree_bin:
        #     pass

        print("Importing tree@", timestamp() - t)
        self.__import(tree_list)

        print("Arranging data@", timestamp() - t)

        data = data[(tree_size + 4):]
        data = PzBinUtil.from_binary(data)

        print("Decompressing@", timestamp() - t)

        r = bytearray()

        is_meta = False
        pz_protocol_ver = -1 # Unset

        reptstage = 0
        reptcount = 0
        reptcc = []

        for i,cc in enumerate(data):
            if reptstage > 0:
                if reptstage == 1:
                    reptcc.extend(cc)

                    if len(reptcc) == 5:
                        reptstage += 1
                        reptcount = reptcc[0] * 64 + reptcc[1] * 16 + reptcc[2] * 4 + reptcc[3]
                        # print("[REPT] count: ", str(reptcount))
                elif reptstage == 2:
                    char = self.tree.get(cc)
                    # print("[REPT] char: ", char)

                    for i in range(reptcount):
                        r.append(char)

                    # print("[REPT dropping...]")
                    reptstage = 0
                    reptcount = 0
                    reptcc = []
                continue

            if cc == self.symbols["[EOA]"]:
                break

            if cc == self.get_sym("PZ META"):
                is_meta = True
            elif cc == self.get_sym("END META"):
                is_meta = False
                if pz_protocol_ver == -1:
                    raise RuntimeError("Decompressing files with no stated protocol is deprecated and may be removed at any time!")

            # In-compression symbols:
            if cc == self.get_sym("REPT"):
                # print("[REPT] entered")
                reptstage = 1

            # print(cc)
            if not self.tree.get(cc) in self.symbols:
                if not is_meta:
                    r.append(self.tree.get(cc))

            if is_meta:
                if cc == self.get_sym("PZ PROTOCOL 0"):
                    pz_protocol_ver = 0

        return bytes(r)

class PzBinUtil:
    def kaboom_char(b):
        '''Converts a single byte int into a list of four values suitable for PzBinUtil.to_binary()'''
        byte_buf = []
        if 255 >= b >= 0:
            byte_buf.append((b & 0b11000000) >> 6)
            byte_buf.append((b & 0b00110000) >> 4)
            byte_buf.append((b & 0b00001100) >> 2)
            byte_buf.append((b & 0b00000011))
            return byte_buf
        else:
            raise ValueError("Must be an integer value between 0 and 255 inclusive.")

    def kaboom_short(b):
        bytes_buf = []
        if not (65535 >= b >= 0):
            raise ValueError("Must be an integer value between 0 and 2^32-1 inclusive.")
        else:
            for i in range(8):
                bytes_buf.append((b & (0b0011 << ((7 - i) * 2))) >> ((7 - i) * 2))
            return bytes_buf

    def kaboom_dword(b):
        bytes_buf = []
        if not (4294967295 >= b >= 0):
            raise ValueError("Must be an integer value between 0 and 2^32-1 inclusive.")
        else:
            for i in range(16):
                bytes_buf.append((b & (0b0011 << ((15 - i) * 2))) >> ((15 - i) * 2))
            return bytes_buf

    def to_binary(c_data:list):
        f_data = []

        for i in c_data:
            f_data.extend(i)

        byte_buf = []
        r = []
        for i in range(len(f_data)):
            byte_buf.append(f_data[i])

            if len(byte_buf) == 4:
                a = byte_buf[3]
                a |= byte_buf[2] << 2
                a |= byte_buf[1] << 4
                a |= byte_buf[0] << 6

                r.append(a)
                byte_buf = []

        if byte_buf:
            while len(byte_buf) < 4:
                byte_buf.append(0)

            a = byte_buf[3]
            a |= byte_buf[2] << 2
            a |= byte_buf[1] << 4
            a |= byte_buf[0] << 6

            r.append(a)

        return bytes(r)

    def from_binary(b_data:bytes):
        r = []
        byte_buf = []
        for b in b_data:
            byte_buf.append((b & 0b11000000) >> 6)
            byte_buf.append((b & 0b00110000) >> 4)
            byte_buf.append((b & 0b00001100) >> 2)
            byte_buf.append((b & 0b00000011))

        char_buf = []
        for i in byte_buf:
            char_buf.append(i)

            if i == 0 and len(char_buf) != 0:
                r.append(char_buf)
                char_buf = []

        if len(char_buf) != 0:
            char_buf.append(0)
            r.append(char_buf)

        return r

if __name__ == "__main__":
    print("Creating compressor...")
    c = PzCompressor()
    with open("en.txt", "rb") as data:
        print("Compressing data...")
        d = c.compress(data.read())
        with open("en.txt.pz", "wb") as pz:
            print("Writing back...")
            # f = PzCompressor()
            # d = f.compress(PzBinUtil.to_binary(d))
            # f.export_tree("test_tree_2.pztree")
            pz.write(d)

    # print("Loading compressed file...")
    # m = PzCompressor()
    # with open("en.txt.pz", "rb") as data:
    #     print("Decompressing file...")
    #     f = m.decompress(data.read())
    #     print("Writing back...")
    #     with open("en2.txt", "wb") as pz:
    #         pz.write(f)
    print("Done!")
