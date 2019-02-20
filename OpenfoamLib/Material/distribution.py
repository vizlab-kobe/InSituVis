def main( new_array , old_array ):
    import numpy as np
    from scipy.stats import norm, entropy, gaussian_kde

    new_value = np.array( new_array )
    old_value = np.array( old_array )

    new_kde = gaussian_kde(new_value)
    old_kde = gaussian_kde(old_value)
    
    new_y = new_kde(np.linspace(3, 45))
    old_y = old_kde(np.linspace(3, 45))
 
    en = entropy( new_y, old_y )
    
    if np.isinf( en ) or en != en:
        return 0.0
    else :
        return en
    
