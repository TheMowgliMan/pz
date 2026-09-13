def find_index(sub, main):
    for i in range(len(main) - len(sub) + 1):
        if main[i : i + len(sub)] == sub:
            return i
    return -1

def findbetween(search_from, search_source, min_size=5):
    search_source = list(search_source)
    search_from = list(search_from)

    for b in range(len(search_source) + 1, 0, -1):
        block = search_source[0 : b]

        idx = find_index(block, search_from)

        if idx >= 0:
            return (idx, len(block))

        if b < min_size:
            break

    return (-1, 0)
