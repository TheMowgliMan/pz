from collections import deque, Counter
import copy

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
        link = self.head
        for i in item:
            if i == 0:
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
        for char,count in c.most_common():
            if firstcount == None:
                firstcount = count

            # print(char, count)

            if count < (firstcount / 100): # Yayy magic number
                self.__add_all_symbols()
                firstcount = -1 # Disables this branch for the rest of the loop

            self.tree.add(char)
            self.fastfind[char] = self.tree.in_order_find(char)

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
        if self.tree == None:
            self.generate_tree(data)

        data = self.__binarize(data)

        r = []
        for c in data:
            if not c in self.fastfind:
                self.tree.add(c)
                self.fastfind[c] = self.tree.in_order_find(c)
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
        tree_size = data[0] + data[1] * 256 + data[2] * 65536 + data[3] * 16777216

        tree_string = bytearray()
        for i in range(tree_size):
            tree_string.append(data[4 + i])
        tree_string = tree_string.decode("utf-8")
        tree_list = tree_string.split("\n")

        print(tree_list)

        self.__import(tree_list)

        data = bytearray(data) # NOTE: maybe unnecesary?
        data = data[tree_size:]
        data = PzBinUtil.from_binary(data)

        r = bytearray()
        is_meta = False
        for cc in data:
            if cc == self.symbols["[EOA]"]:
                break

            if cc == self.get_sym("PZ META"):
                is_meta = True
            elif cc == self.get_sym("END META"):
                is_meta = False

            if not self.tree.get(cc) in self.symbols:
                if not is_meta:
                    r.append(self.tree.get(cc))

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

    def kaboom_dword(b):
        bytes_buf = []
        if not (4294967295 >= b >= 0):
            ValueError("Must be an integer value between 0 and 2^32-1 inclusive.")
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
        byte_buf = deque()
        for b in b_data:
            byte_buf.append((b & 0b11000000) >> 6)
            byte_buf.append((b & 0b00110000) >> 4)
            byte_buf.append((b & 0b00001100) >> 2)
            byte_buf.append((b & 0b00000011))

            while 0 in byte_buf:
                char_buf = []
                while not 0 in char_buf:
                    char_buf.append(byte_buf.popleft())

                r.append(char_buf)

        if byte_buf:
            byte_buf.append(0)
            while 0 in byte_buf:
                char_buf = []
                while not 0 in char_buf:
                    char_buf.append(byte_buf.popleft())

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
        print("Exporting tree...")
    print("Done!")
