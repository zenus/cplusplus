#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
using namespace std;

//#define TEST

struct bound {
  // begin and end indice of an array, a valid index is belong [b, e)
  int b, e; 
  bound(int b_ = 0, int e_ = 0) {
    b = b_;
    e = e_;
  }
};

//
// min heap operations
//
void shiftdown(const vector<vector<int> >& a, const vector<bound>& s, vector<int>& heap, int len, int start = 0)
{
  int i = start, j = 2*start+1;
  int p = heap[i];
  int key = a[p][s[p].b];
  while (j < len) {
    int pj = heap[j];
    if (j+1 < len) {
      int pj1 = heap[j+1];
      if (a[pj][s[pj].b] > a[pj1][s[pj1].b]) {
        j++; 
        pj = pj1;
      }
    }
    if (key <= a[pj][s[pj].b]) {
      break;
    }
    heap[i] = heap[j];
    i = j;
    j = i*2+1;
  }
  heap[i] = p;
}

void createheap(const vector<vector<int> >& a, const vector<bound>& s, vector<int>& heap, int len)
{
  int i = len/2-1;
  while (i >= 0) {
    shiftdown(a, s, heap, len, i);
    --i;
  }
}

void printheap(const vector<int>& heap, const vector<bound>& s, int heaplen)
{
#ifdef TEST
    cout << "heap: " << endl;
    for (int i = 0; i < heaplen; i++) {
      cout << heap[i] << ":" << s[heap[i]].b << " ";
    }
    cout << endl;
#endif
}

// 
// interface. the |k|th value of arrays |a| is reserved in |val| 
//
bool findk(const vector<vector<int> >& a, int k, int *val)
{
  assert(val);

  int M = a.size();
  int validM = 0;
  vector<bound> s;
  int n = 0;
  for (int i = 0; i < M; i++) {
    s.push_back(bound(0, a[i].size()));
    n += a[i].size();
    if (a[i].size() > 0) {
      validM++;
    }
  }
  
  if (k > n || k < 1) {
    return false;
  }

  while (k >= validM && validM > 1) {
    int maxi = -1, mini = -1;
    int maxm, minm;
    int summ = 0;
    double sumr = 0;
    for (int i = 0; i < M; i++) {
      if (s[i].b < s[i].e) {
        int len = s[i].e - s[i].b - 1;
        double dm = 1;
        if (len > 0) {
          dm += (double)(k-validM)*len/(n - validM);
        }
        int m = (int)dm;
        sumr += dm;
        summ += m;
        if (sumr - summ >= 0.99) {
          m++;
          summ++;
        }
        if (maxi == -1 || a[i][s[i].b + m - 1] > a[maxi][maxm-1]) {
          maxi = i;
          maxm = s[i].b + m;
        } 
        if (mini == -1 || a[i][s[i].b + m - 1] < a[mini][minm-1]) {
          mini = i;
          minm = s[i].b + m;
        }
      }
    }
    if (summ != k) {
      cerr << "k summ sumr: " << k << " " << summ << " " << sumr << endl;
    }
    k -= minm - s[mini].b;
    n -= minm - s[mini].b + s[maxi].e - maxm;
    s[mini].b = minm;
    s[maxi].e = maxm;
    if (s[mini].b >= s[mini].e) {
      validM--;
    }
  }

  // there remains validM candidate arrays and k equals to validM-1, so we need a heap to find the k-th value
  if (validM > 1) {
    vector<int> minheap;
    int heaplen = 0;
    for (int i = 0; i < M; i++) {
      if (s[i].b < s[i].e) {
        minheap.push_back(i);
        heaplen++;
      }
    }
    createheap(a, s, minheap, heaplen);
    printheap(minheap, s, heaplen);
    int p = minheap[0];
    for (int i = 1; i < k; i++) {
      if (s[p].b + 1 < s[p].e) {
        s[p].b++;
      } else {
        minheap[0] = minheap[--heaplen];
      }
      shiftdown(a, s, minheap, heaplen);
      p = minheap[0];
    }
    printheap(minheap, s, heaplen);
    *val = a[p][s[p].b];
  } else {
    for (int i = 0; i < M; i++) {
      if (s[i].b < s[i].e) {
        *val = a[i][s[i].b+k-1];
        break;
      }
    }
  }
  return true;
}


//
// helper functions for test
//
void print(const vector<vector<int> >& arrays)
{
#ifdef TEST
  for (int i = 0; i < arrays.size(); i++) {
    for (int j = 0; j < arrays[i].size(); j++) {
      cout << " " << arrays[i][j];
    }
    cout << endl;
  }
#endif
}

vector<int> merge(const vector<vector<int> >& arrays)
{
  vector<int> ret;
  for (int i = 0; i < arrays.size(); i++) {
    for (int j = 0; j < arrays[i].size(); j++) {
      ret.push_back(arrays[i][j]);
    }
  }
  sort(ret.begin(), ret.end());
  return ret;
}

int main(void)
{
  int n, k, val;
  while (cin >> n >> k) {
    vector<vector<int> > arrays;
    for (int i = 0; i < n; i++) {
      int m;
      cin >> m;
      vector<int> a;
      for (int j = 0; j < m; j++) {
        int v;
        cin >> v;
        a.push_back(v);
      }
      //sort(a.begin(), a.end());
      arrays.push_back(a);
    }
    cout << "n: " << n << " k: " << k << endl;
    print(arrays);
    
    vector<int> all = merge(arrays);
    if (k <= all.size() && k > 0) {
      cout << "the right answer is: " << all[k-1] << endl;
    } else {
      cout << "k is out of bound" << endl;
    }
    if (findk(arrays, k, &val)) {      
      cout << "The " << k << "th value is: " << val << endl;
    } else {
      cout << "Cannot find the " << k << "th value" << endl;
    }
    
  }
  return 0;
}