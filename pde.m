1;
function rv = pdematrix(n)
  B = gallery("tridiag",n,-1,4,-1);

  tmp = [B,-eye(n)];
  for t = 1:n-2
    tmp = [tmp,zeros(n)];
  end
  rv = tmp;
  
  for i = 2:n-1
    tmp = [ -eye(n), B, -eye(n) ];
    #i
    for t = 1:i-2
      tmp = [zeros(n),tmp];
    end
    for t = i+2:n
      tmp = [tmp,zeros(n)];
    end
    rv = [rv; tmp];
  end

  tmp = [-eye(n),B];
  for t = 1:n-2
    tmp = [zeros(n),tmp];
  end
  rv = [rv; tmp];

  rv = full(rv);
endfunction

function rv = pdeedges(b)
  n = max(size(b));
  rv = zeros(n*n,1);
  rv(1:n) = b;
  for i = 0:n-1
    t = n*i+1;
    rv(t) = rv(t) + b(i+1);
    t = n*i+n;
    rv(t) = rv(t) + b(n);
  end
  rv(end-n+1:end) = rv(end-n+1:end) + ones(n,1)*b(end);
endfunction

function b = pdebvect(n)
  n3 = floor(n/3);
  b = [ones(1,n3),2:n3+1,ones(1,n-2*n3)*(n3+2)];
end

# n = 20;

b = pdebvect(n);
bb = pdeedges(b);
A = pdematrix(n);
%rvm = inv(A) * bb;
%rvm = reshape(rvm,n,n);
% mesh(rvm);
rv = reshape(bb,n,n);
