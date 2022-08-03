from ltypes import i32, f64


@overload
def mean(x: list[i32]) -> f64:
    """
    Returns the arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        sum += x[i]
    ans: f64
    ans = sum/k
    return ans

@overload
def mean(x: list[f64]) -> f64:
    """
    Returns the arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32
    for i in range(k):
        sum += x[i]
    ans: f64
    ans = sum/k
    return ans

@overload
def mean(x: list[i32]) -> i32:
    """
    Returns the arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0
    sum: i32
    sum = 0

    i: i32
    for i in range(k):
        sum += x[i]

    if sum%k == 0:
        ans: i32
        ans = sum/k
        return ans


@overload
def fmean(x: list[i32]) -> f64:
    """
    Returns the floating type arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32

    for i in range(k):
        sum += x[i]
    ans: f64
    ans = sum/k
    return ans


@overload
def fmean(x: list[f64]) -> f64:
    """
    Returns the floating type arithmetic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32
    for i in range(k):
        sum += x[i]
    ans: f64
    ans = sum/k
    return ans


@overload
def geometric_mean(x: list[i32]) -> f64:
    """
    Returns the geometric mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    product: f64
    product = 1.0
    i: i32
    for i in range(k):
        if x[i] < 1:
            raise ValueError('geometric mean requires a non-empty dataset  containing positive numbers')
        product *= x[i]
    ans: f64
    ans = product**(1/k)
    return ans

@overload
def harmonic_mean(x: list[i32]) -> f64:
    """
    Returns the harmonic mean of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    sum: f64
    sum = 0.0
    i: i32
    for i in range(k):
        if x[i] < 0:
            raise ValueError('harmonic mean does not support negative values')
        if x[i] ==0:
            return 0.0
        sum += 1 / x[i]
    ans: f64
    ans = k/sum
    return ans

@overload
def median(x: list[i32]) -> f64:
    """
    Returns the median of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    i: i32
    j: i32
    for i in range(k):
        for j in range(i+1,k):
            if x[i]>x[j]:
                x[i],x[j]=x[j],x[i]
    ans: f64
    if k%2==1:
        medianidx: i32
        medianidx = k/2
        ans = float(x[medianidx])
    else:
        lowmedidx: i32
        highmedidx: i32
        lowmedidx = (k/2)-1
        highmedidx = k/2
        ans = float((x[lowmedidx] + x[highmedidx])/2)
    return ans

@overload
def median(x: list[f64]) -> f64:
    """
    Returns the median of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    i: i32
    j: i32
    for i in range(k):
        for j in range(i+1,k):
            if x[i]>x[j]:
                x[i],x[j]=x[j],x[i]
    ans: f64
    if k%2==1:
        medianidx: i32
        medianidx = k/2
        ans = x[medianidx]
    else:
        lowmedidx: i32
        highmedidx: i32
        lowmedidx = (k/2)-1
        highmedidx = k/2
        ans = (x[lowmedidx] + x[highmedidx])/2
    return ans

@overload
def median(x: list[i32]) -> i32:
    """
    Returns the median of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0
    i: i32
    j: i32
    for i in range(k):
        for j in range(i+1,k):
            if x[i]>x[j]:
                x[i],x[j]=x[j],x[i]
    ans: i32
    if k%2==1:
        medianidx: i32
        medianidx = k/2
        ans = x[medianidx]
        return ans
    else:
        lowmedidx: i32
        highmedidx: i32
        lowmedidx = (k/2)-1
        highmedidx = k/2
        if (x[lowmedidx] + x[highmedidx])%2 ==0:
            ans = (x[lowmedidx] + x[highmedidx])/2
            return ans

@overload
def median_low(x: list[i32]) -> i32:
    """
    Returns the lowmedian of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0
    i: i32
    j: i32
    for i in range(k):
        for j in range(i+1,k):
            if x[i]>x[j]:
                x[i],x[j]=x[j],x[i]
    ans: i32
    if k%2==1:
        medianidx: i32
        medianidx = k/2
        ans = x[medianidx]
    else:
        lowmedidx: i32
        lowmedidx = (k/2)-1
        ans = x[lowmedidx]
    return ans

@overload
def median_low(x: list[f64]) -> f64:
    """
    Returns the low median of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    i: i32
    j: i32
    for i in range(k):
        for j in range(i+1,k):
            if x[i]>x[j]:
                x[i],x[j]=x[j],x[i]
    ans: f64
    if k%2==1:
        medianidx: i32
        medianidx = k/2
        ans = x[medianidx]
    else:
        lowmedidx: i32
        lowmedidx = (k/2)-1
        ans = x[lowmedidx]
    return ans


@overload
def median_high(x: list[i32]) -> i32:
    """
    Returns the high median of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0
    i: i32
    j: i32
    for i in range(k):
        for j in range(i+1,k):
            if x[i]>x[j]:
                x[i],x[j]=x[j],x[i]
    ans: i32
    if k%2==1:
        medianidx: i32
        medianidx = k/2
        ans = x[medianidx]
    else:
        highmedidx: i32
        highmedidx = (k/2)
        ans = x[highmedidx]
    return ans

@overload
def median_high(x: list[f64]) -> f64:
    """
    Returns the high median of a data sequence of numbers
    """
    k: i32 = len(x)
    if k == 0:
        return 0.0
    i: i32
    j: i32
    for i in range(k):
        for j in range(i+1,k):
            if x[i]>x[j]:
                x[i],x[j]=x[j],x[i]
    ans: f64
    if k%2==1:
        medianidx: i32
        medianidx = k/2
        ans = x[medianidx]
    else:
        highmedidx: i32
        highmedidx = (k/2)
        ans = x[highmedidx]
    return ans