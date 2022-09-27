def test_issue_1153():
    start: list[i32] = [-10, 0, 10]
    stop: list[i32] = [20, 0, -20]
    a: i32; b: i32; c: i32; count: i32; j: i32

    for a in range(len(start)):
        for b in range(len(stop)):
            count = 0
            for j in range(start[a], stop[b], -2):
                count += 1
            assert count == max(0, (start[a] - stop[b]) // 2)

    for a in range(len(start)):
        for b in range(len(stop)):
            count = 0
            for j in range(start[a], stop[b], -1):
                count += 1
            assert count == max(0, start[a] - stop[b])


    for a in range(len(start)):
        for b in range(len(stop)):
            count = 0
            for j in range(start[a], stop[b], 2):
                count += 1
            assert count == max(0, (stop[b] - start[a]) // 2)

    for a in range(len(start)):
        for b in range(len(stop)):
            count = 0
            for j in range(start[a], stop[b], 1):
                count += 1
            assert count == max(0, stop[b] - start[a])



test_issue_1153()
