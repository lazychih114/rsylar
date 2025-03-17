#include<iostream>
using namespace std;
class Solution {
    public:
        string addBinary(string a, string b) {
            
            string res;
            
            int sz=res.size()-1;
            int len1=a.size()-1;
            int len2=b.size()-1;
            for(int i=0;i<=len1;i++) a[i] -= '0';
            for(int i=0;i<=len2;i++) b[i]-='0';
            res.resize(max(len1,len2)+10);
            cout << " start " << endl;
            char carry=0;
            while(len1>=0&&len2>=0)
            {
                cout << len1 << " " << len2 << " " << sz << endl;
                //cout << res[sz] << endl;
                cout << (int)(unsigned char)a[len1] << " " << (unsigned int)b[len2] << " " << (unsigned int)res[sz] << endl;
                if(a[len1]&&b[len2]){res[sz]=carry;carry=1;}
                else if(!a[len1]&&!b[len2]){res[sz]=carry;}
                else if(a[len1]||b[len2])
                {
                    res[sz] = carry?0:1;
                    carry = carry?1:0;
                }
                cout << (int)a[len1] << " " << (int)b[len2] << " " << (int)res[sz] << " "<< (int)carry<<endl;
                len1--;
                len2--;
                sz--;
                cout << len1 << " ** " << len2 << " ** " << sz << endl;
            }
            cout << "fdsfs";
            while(len1>=0)
            {
                res[sz]=(carry+a[len1])%2;
                carry = (carry+a[len1])/2;
                len1--;
                sz--;
            }
            while(len2>=0)
            {
                res[sz]=(carry+b[len2])%2;
                carry = (carry+b[len2])/2;
                len2--;
                sz--;
            }
            if(carry)
            {
                res[sz]=1;
            }
            else sz++;
            string ans = res.substr(sz);
            for(auto &i:ans) i+='0';
            return ans;
    
        }
    };
int main()
{
    Solution s;
    cout << s.addBinary("11","1") << endl;
    return 0;
}
