import random
import math
import time
def gcd(a, b):
	while(b > 0):
		a, b = b, a%b
	return a

def encryption(m, n, g, nSquare):
	r = random.randint(2, n)
	while(gcd(r,n)!=1):
		r = r + 1 if (r < n - 1) else 2
	print("pow(g, m): {0}".format(pow(g, m)))
	print("pow(r, n): {0}".format(pow(r, n)))
	print("pow(g, m, nSquare): {0}".format(pow(g, m, nSquare)))
	print("pow(r, n, nSquare): {0}".format(pow(r, n, nSquare)))
	c = (pow(g, m, nSquare) * pow(r, n, nSquare)) % nSquare
	return c

def lcm(a,b):
	return a * b / gcd(a, b)

def functionL(u,n):
	return (u-1)//n

def findU(k,n):
	s, old_s = 0, 1
	t, old_t = 1, 0
	r, old_r = n, k

	while r != 0:
		quotient = old_r // r
		old_r, r = r, old_r - quotient * r
		old_s, s = s, old_s - quotient * s
		old_t, t = t, old_t - quotient * t


	return  old_s % n

def decryption(c, lamda, nSquare, u, n):
	return (functionL(pow(c,lamda,nSquare), n) * u) % n

def start(p, q, m):
	n = p * q
	nSquare = n * n
	g = 2
	lamda = int(lcm(p - 1, q - 1))
	k = functionL(pow(g, lamda, nSquare), n)
	while((gcd(g,n) != 1) or (gcd(k,n) != 1)):
		g = g + 1 if (g < nSquare - 1) else 2
		k = functionL(pow(g, lamda, nSquare), n)
	c = encryption(m,n,g,nSquare)

	u = findU(k,n)

	mFromc = decryption(c, lamda, nSquare, u, n)
	m1 = 3
	c1 = encryption(m1,n,g,nSquare)
	m1Fromc1 = decryption(c1, lamda, nSquare, u, n)
	m2 = m + m1
	c2 = c * c1
	m2Fromc2 = decryption(c2, lamda, nSquare, u, n)
	m3 = m * m1
	c3 = pow(c, m1, nSquare)
	m3Fromc3 = decryption(c3, lamda, nSquare, u, n)
	m4 = m - m1
	c1Negitive = findU(c1,nSquare)
	c4 = c * c1Negitive
	m4Fromc4 = decryption(c4, lamda, nSquare, u, n)
	print("m = {0}".format(m))
	print("c = {0}".format(c))
	print("mFromc = {0}".format(mFromc))
	print("m1 = {0}".format(m1))
	print("c1 = {0}".format(c1))
	print("m1Fromc1 = {0}".format(m1Fromc1))
	print("m2 = m + m1 = {0}".format(m2))
	print("c2 = {0}".format(c2))
	print("m2Fromc2 = {0}".format(m2Fromc2))
	print("m3 = m * m1 = {0}".format(m3))
	print("c3 = {0}".format(c3))
	print("m3Fromc3 = {0}".format(m3Fromc3))
	print("m4 = m - m1 = {0}".format(m4))
	print("c4 = {0}".format(c4))
	print("m4Fromc4 = {0}".format(m4Fromc4))

start(3,5,4)
